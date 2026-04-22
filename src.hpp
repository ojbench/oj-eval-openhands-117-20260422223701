#ifndef SRC_HPP
#define SRC_HPP
#include <cstddef>
/**
 * 枚举类，用于枚举可能的置换策略
 */
enum class ReplacementPolicy { kDEFAULT = 0, kFIFO, kLRU, kMRU, kLRU_K };

/**
 * @brief 该类用于维护每一个页对应的信息以及其访问历史，用于在尝试置换时查询需要的信息。
 */
class PageNode {
public:
  PageNode() = default;

  void Init(std::size_t page_id, std::size_t added_seq, std::size_t first_time,
            std::size_t k, std::size_t *lastk_buf) {
    page_id_ = page_id;
    added_seq_ = added_seq;
    first_time_ = first_time;
    k_ = k;
    lastk_ = lastk_buf;
    count_ = 0;
  }

  void AssignFrom(const PageNode &src, std::size_t *dest_buf) {
    page_id_ = src.page_id_;
    added_seq_ = src.added_seq_;
    first_time_ = src.first_time_;
    k_ = src.k_;
    count_ = src.count_;
    lastk_ = dest_buf;
    if (k_ && dest_buf) {
      std::size_t c = count_ < k_ ? count_ : k_;
      for (std::size_t i = 0; i < c; ++i) dest_buf[i] = src.lastk_[i];
    }
  }

  void Visit(std::size_t now) {
    std::size_t cap = k_ == 0 ? 0 : k_;
    if (cap == 0) return;
    std::size_t lim = count_ < cap ? count_ : (cap - 1);
    for (std::size_t i = lim; i > 0; --i) lastk_[i] = lastk_[i - 1];
    lastk_[0] = now;
    if (count_ < cap) ++count_;
  }

  std::size_t PageId() const { return page_id_; }
  std::size_t AddedSeq() const { return added_seq_; }
  std::size_t FirstTime() const { return first_time_; }
  std::size_t LastTime() const { return count_ > 0 ? lastk_[0] : 0; }
  bool HasK() const { return k_ != 0 && count_ >= k_; }
  std::size_t KthTime() const { return (k_ != 0 && HasK()) ? lastk_[k_ - 1] : 0; }

private:
  std::size_t page_id_{};
  std::size_t added_seq_{};
  std::size_t first_time_{};
  std::size_t k_{};
  std::size_t count_{};
  std::size_t *lastk_{};  // length k_
};

class ReplacementManager {
public:
  constexpr static std::size_t npos = (std::size_t)-1;

  ReplacementManager() = delete;

  /**
   * @brief 初始化整个类
   * @param max_size 缓存池可以容纳的页数量的上限
   * @param k LRU-K所基于的常数k，在类销毁前不会变更
   * @param default_policy 在置换时，如果没有显式指示，则默认使用default_policy作为策略
   * @note 我们将保证default_policy的值不是ReplacementPolicy::kDEFAULT。
   */
  ReplacementManager(std::size_t max_size, std::size_t k, ReplacementPolicy default_policy)
      : max_size_(max_size), k_(k), default_policy_(default_policy) {
    nodes_ = max_size_ ? new PageNode[max_size_] : nullptr;
    pool_ = (max_size_ && k_)
                ? new std::size_t[max_size_ * k_]
                : nullptr;
    for (std::size_t i = 0; i < max_size_; ++i) {
      if (k_)
        nodes_[i].Init(0, 0, 0, k_, pool_ + i * k_);
      else
        nodes_[i].Init(0, 0, 0, 0, nullptr);
    }
  }

  /**
   * @brief 析构函数
   * @note 我们将对代码进行Valgrind Memcheck，请保证你的代码不发生内存泄漏
   */
  ~ReplacementManager() {
    delete[] pool_;
    delete[] nodes_;
    nodes_ = nullptr;
    pool_ = nullptr;
  }

  /**
   * @brief 重设当前默认的缓存置换政策
   * @param default_policy 新的默认政策，保证default_policy不是ReplacementPolicy::kDEFAULT
   */
  void SwitchDefaultPolicy(ReplacementPolicy default_policy) { default_policy_ = default_policy; }

  /**
   * @brief 访问某个页面。
   * @param page_id 访问页的编号
   * @param evict_id 需要被置换的页编号，如果不需要置换请将其设置为npos
   * @param policy 如果需要置换，那么置换所基于的策略
   * (a) 若访问的页已经在缓存池中，那么直接记录其访问信息。
   * (b) 若访问的页不在缓存池中，那么：
   *    1. 若缓存池已满，就从中依照policy置换一个页（彻底删除其对应节点），并将新访问的页加入缓存池，记录其访问
   *    2. 若缓存池未满，则直接将其加入缓存池并记录其访问
   * @note 我们不保证page_id在调用间连续，也不保证page_id的范围，只保证page_id在std::size_t内
   */
  void Visit(std::size_t page_id, std::size_t &evict_id, ReplacementPolicy policy = ReplacementPolicy::kDEFAULT) {
    if (policy == ReplacementPolicy::kDEFAULT) policy = default_policy_;
    // search
    std::size_t idx = npos;
    for (std::size_t i = 0; i < size_; ++i) {
      if (nodes_[i].PageId() == page_id) { idx = i; break; }
    }
    ++time_counter_;
    if (idx != npos) {
      evict_id = npos;
      nodes_[idx].Visit(time_counter_);
      return;
    }
    // not found
    if (Full()) {
      std::size_t victim_id = TryEvict(policy);
      evict_id = victim_id;
      // remove victim
      for (std::size_t i = 0; i < size_; ++i) {
        if (nodes_[i].PageId() == victim_id) {
          // move last into i
          if (i != size_ - 1) {
            nodes_[i].AssignFrom(nodes_[size_ - 1], k_ ? (pool_ + i * k_) : nullptr);
          }
          --size_;
          break;
        }
      }
    } else {
      evict_id = npos;
    }
    // insert new page at end
    if (size_ < max_size_) {
      std::size_t i = size_;
      // Re-init the slot i with its associated pool segment
      std::size_t *buf = k_ ? (pool_ + i * k_) : nullptr;
      nodes_[i].Init(page_id, ++seq_counter_, time_counter_, k_, buf);
      nodes_[i].Visit(time_counter_);
      ++size_;
    }
  }

  /**
   * @brief 强制地删除特定的页（无论缓存池是否已满）
   * @param page_id 被删除页的编号
   * @return 如果成功删除，则返回true; 如果该页不存在于缓存池中，则返回false
   * 如果page_id存在于缓存池中，则删除它；否则，直接返回false
   */
  bool RemovePage(std::size_t page_id) {
    for (std::size_t i = 0; i < size_; ++i) {
      if (nodes_[i].PageId() == page_id) {
        if (i != size_ - 1) {
          nodes_[i].AssignFrom(nodes_[size_ - 1], k_ ? (pool_ + i * k_) : nullptr);
        }
        --size_;
        return true;
      }
    }
    return false;
  }

  /**
   * @brief 查询特定策略下首先被置换的页
   * @param policy 置换策略
   * @return 当前策略下会被置换的页的编号。若缓存池没满，则返回npos
   * 不对缓存池做任何修改，只查询在需要置换的情况下，基于给定的政策，应该置换哪个页。
   * @note 如果缓存池没有满，请直接返回npos
   */
  [[nodiscard]] std::size_t TryEvict(ReplacementPolicy policy = ReplacementPolicy::kDEFAULT) const {
    if (!Full()) return npos;
    if (policy == ReplacementPolicy::kDEFAULT) policy = default_policy_;
    if (size_ == 0) return npos;
    std::size_t best_idx = 0;
    auto better_fifo = [&](std::size_t a, std::size_t b) {
      // smaller added_seq first; tie-break by smaller PageId
      if (nodes_[a].AddedSeq() != nodes_[b].AddedSeq())
        return nodes_[a].AddedSeq() < nodes_[b].AddedSeq();
      return nodes_[a].PageId() < nodes_[b].PageId();
    };
    auto better_lru = [&](std::size_t a, std::size_t b) {
      std::size_t ta = nodes_[a].LastTime();
      std::size_t tb = nodes_[b].LastTime();
      if (ta != tb) return ta < tb;  // older last access
      return nodes_[a].PageId() < nodes_[b].PageId();
    };
    auto better_mru = [&](std::size_t a, std::size_t b) {
      std::size_t ta = nodes_[a].LastTime();
      std::size_t tb = nodes_[b].LastTime();
      if (ta != tb) return ta > tb;  // newer last access
      return nodes_[a].PageId() < nodes_[b].PageId();
    };
    auto better_lruk = [&](std::size_t a, std::size_t b) {
      bool haka = nodes_[a].HasK();
      bool hakb = nodes_[b].HasK();
      if (haka != hakb) return !haka; // lacking k preferred
      if (!haka) {
        // both lacking: earliest first access
        if (nodes_[a].FirstTime() != nodes_[b].FirstTime())
          return nodes_[a].FirstTime() < nodes_[b].FirstTime();
        return nodes_[a].PageId() < nodes_[b].PageId();
      }
      // both have k: smaller kth time
      std::size_t ka = nodes_[a].KthTime();
      std::size_t kb = nodes_[b].KthTime();
      if (ka != kb) return ka < kb;
      return nodes_[a].PageId() < nodes_[b].PageId();
    };

    for (std::size_t i = 1; i < size_; ++i) {
      bool better = false;
      switch (policy) {
        case ReplacementPolicy::kFIFO: better = better_fifo(i, best_idx); break;
        case ReplacementPolicy::kLRU: better = better_lru(i, best_idx); break;
        case ReplacementPolicy::kMRU: better = better_mru(i, best_idx); break;
        case ReplacementPolicy::kLRU_K: better = better_lruk(i, best_idx); break;
        default: better = false; break;
      }
      if (better) best_idx = i;
    }
    return nodes_[best_idx].PageId();
  }

  /**
   * @brief 返回当前缓存管理器是否为空。
   */
  [[nodiscard]] bool Empty() const { return size_ == 0; }

  /**
   * @brief 返回当前缓存管理器是否已满（即是否页数量已经达到上限）
   */
  [[nodiscard]] bool Full() const { return size_ >= max_size_; }

  /**
   * @brief 返回当前缓存管理器中页的数量
   */
  [[nodiscard]] std::size_t Size() const { return size_; }

private:
  std::size_t max_size_{};
  std::size_t k_{};
  ReplacementPolicy default_policy_{ReplacementPolicy::kFIFO};
  std::size_t size_{};
  std::size_t time_counter_{};  // global increasing time
  std::size_t seq_counter_{};   // FIFO sequence counter

  PageNode *nodes_{};           // length max_size_
  std::size_t *pool_{};         // length max_size_ * k_
};
#endif