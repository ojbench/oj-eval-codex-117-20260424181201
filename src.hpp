// Submission entry: full implementation
#ifndef SRC_HPP
#define SRC_HPP
enum class ReplacementPolicy { kDEFAULT = 0, kFIFO, kLRU, kMRU, kLRU_K };
class PageNode {
public:
  PageNode() = default;
  explicit PageNode(std::size_t id) : page_id_(id) {}
  std::size_t PageId() const { return page_id_; }
  void SetPageId(std::size_t id) { page_id_ = id; }
  void RecordVisit(std::size_t t) {
    if (visit_count_ == 0) arrival_time_ = t;
    if (visit_count_ < max_keep_) {
      for (std::size_t i = visit_count_; i > 0; --i) visits_[i] = visits_[i - 1];
      visits_[0] = t;
      ++visit_count_;
    } else {
      for (std::size_t i = max_keep_ - 1; i > 0; --i) visits_[i] = visits_[i - 1];
      visits_[0] = t;
    }
  }
  bool GetKthRecent(std::size_t k, std::size_t &out) const {
    if (k == 0 || k > visit_count_) return false;
    out = visits_[k - 1];
    return true;
  }
  bool GetOldest(std::size_t &out) const {
    if (visit_count_ == 0) return false;
    out = visits_[visit_count_ - 1];
    return true;
  }
  std::size_t ArrivalTime() const { return arrival_time_; }
  bool GetMostRecent(std::size_t &out) const {
    if (visit_count_ == 0) return false;
    out = visits_[0];
    return true;
  }
private:
  std::size_t page_id_{};
  static constexpr std::size_t max_keep_ = 32;
  std::size_t visits_[max_keep_]{};
  std::size_t visit_count_{};
  std::size_t arrival_time_{};
};
class ReplacementManager {
public:
  constexpr static std::size_t npos = -1;
  ReplacementManager() = delete;
  ReplacementManager(std::size_t max_size, std::size_t k, ReplacementPolicy default_policy) {
    max_size_ = max_size; k_ = k; default_policy_ = default_policy; size_ = 0; time_ = 0;
    nodes_ = (PageNode*)::operator new[](max_size_ * sizeof(PageNode));
    occupied_ = (bool*)::operator new[](max_size_ * sizeof(bool));
    for (std::size_t i = 0; i < max_size_; ++i) { new (&nodes_[i]) PageNode(); occupied_[i] = false; }
  }
  ~ReplacementManager() {
    if (nodes_) { for (std::size_t i = 0; i < max_size_; ++i) { nodes_[i].~PageNode(); } ::operator delete[](nodes_); nodes_ = nullptr; }
    if (occupied_) { ::operator delete[](occupied_); occupied_ = nullptr; }
  }
  void SwitchDefaultPolicy(ReplacementPolicy default_policy) { default_policy_ = default_policy; }
  void Visit(std::size_t page_id, std::size_t &evict_id, ReplacementPolicy policy = ReplacementPolicy::kDEFAULT) {
    evict_id = npos;
    for (std::size_t i = 0; i < max_size_; ++i) if (occupied_[i] && nodes_[i].PageId() == page_id) { nodes_[i].RecordVisit(++time_); return; }
    if (size_ < max_size_) {
      for (std::size_t i = 0; i < max_size_; ++i) if (!occupied_[i]) { occupied_[i] = true; nodes_[i] = PageNode(page_id); nodes_[i].RecordVisit(++time_); ++size_; return; }
      return;
    }
    std::size_t victim = TryEvict(policy);
    if (victim == npos) return;
    evict_id = victim;
    for (std::size_t i = 0; i < max_size_; ++i) if (occupied_[i] && nodes_[i].PageId() == victim) {
      occupied_[i] = false; nodes_[i] = PageNode(page_id); occupied_[i] = true; nodes_[i].RecordVisit(++time_); return; }
  }
  bool RemovePage(std::size_t page_id) {
    for (std::size_t i = 0; i < max_size_; ++i) if (occupied_[i] && nodes_[i].PageId() == page_id) { occupied_[i] = false; if (size_ > 0) --size_; return true; }
    return false;
  }
  [[nodiscard]] std::size_t TryEvict(ReplacementPolicy policy = ReplacementPolicy::kDEFAULT) const {
    if (size_ < max_size_) return npos;
    ReplacementPolicy p = (policy == ReplacementPolicy::kDEFAULT) ? default_policy_ : policy;
    std::size_t selected_id = npos; std::size_t selected_val = 0; bool selected_set = false;
    for (std::size_t i = 0; i < max_size_; ++i) {
      if (!occupied_[i]) continue; const PageNode &node = nodes_[i];
      if (p == ReplacementPolicy::kFIFO) { std::size_t arr = node.ArrivalTime(); if (!selected_set || arr < selected_val) { selected_set = true; selected_val = arr; selected_id = node.PageId(); } }
      else if (p == ReplacementPolicy::kLRU) { std::size_t recent; node.GetMostRecent(recent); if (!selected_set || recent < selected_val) { selected_set = true; selected_val = recent; selected_id = node.PageId(); } }
      else if (p == ReplacementPolicy::kMRU) { std::size_t recent; node.GetMostRecent(recent); if (!selected_set || recent > selected_val) { selected_set = true; selected_val = recent; selected_id = node.PageId(); } }
      else { std::size_t kth = 0, oldest = node.ArrivalTime(); bool has_k = node.GetKthRecent(k_, kth); if (!selected_set) { selected_set = true; selected_id = node.PageId(); sel_has_k_ = has_k; sel_kth_ = kth; sel_oldest_ = oldest; }
             else { if (!sel_has_k_ && !has_k) { if (oldest < sel_oldest_) { selected_id = node.PageId(); sel_oldest_ = oldest; } }
                    else if (!sel_has_k_ && has_k) { }
                    else if (sel_has_k_ && !has_k) { selected_id = node.PageId(); sel_has_k_ = false; sel_oldest_ = oldest; }
                    else { if (kth < sel_kth_) { selected_id = node.PageId(); sel_kth_ = kth; } } } }
    }
    return selected_id;
  }
  [[nodiscard]] bool Empty() const { return size_ == 0; }
  [[nodiscard]] bool Full() const { return size_ == max_size_; }
  [[nodiscard]] std::size_t Size() const { return size_; }
private:
  std::size_t max_size_{}; std::size_t size_{}; std::size_t k_{}; ReplacementPolicy default_policy_{ReplacementPolicy::kFIFO};
  PageNode *nodes_{}; bool *occupied_{}; mutable std::size_t time_{};
  mutable bool sel_has_k_{}; mutable std::size_t sel_kth_{}; mutable std::size_t sel_oldest_{};
};
#endif
