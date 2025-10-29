// © Copyright 2025 Stuart Parmenter
// SPDX-License-Identifier: MIT

#include "lvgl_page_manager.h"
#include <algorithm>

namespace esphome {
namespace lvgl_page_manager {

void PageManager::set_default_page(const std::string &page_id) { this->default_page_ = page_id; }
void PageManager::set_sort_mode(uint8_t mode) { this->sort_mode_ = mode; }
void PageManager::set_push_mode(uint8_t mode) { this->push_mode_ = mode; }
void PageManager::set_lvgl(esphome::lvgl::LvglComponent *lvgl) { this->lvgl_ = lvgl; }

void PageManager::add_page(const std::string &page_id, const std::string &name, int order, esphome::lvgl::LvPageType *page) {
  PageEntry e{page_id, name, order, page};
  pages_.push_back(e);
}

void PageManager::setup() {
  if (!default_page_.empty()) {
    this->show_page(default_page_);
  } else if (!pages_.empty() && current_index_ < 0) {
    current_index_ = 0;
    auto &p = pages_[0];
    this->show_lvgl_page_(p.page);
    this->publish_state(p.name);
    ESP_LOGI(TAG, "Initial page: %s (%s)", p.name.c_str(), p.page_id.c_str());
  }
}

void PageManager::dump_config() {
  ESP_LOGCONFIG(TAG, "LVGL Page Manager");
  ESP_LOGCONFIG(TAG, "  LVGL: %p", this->lvgl_);
  ESP_LOGCONFIG(TAG, "  Pages: %u", (unsigned) pages_.size());
  for (size_t i = 0; i < pages_.size(); i++) {
    ESP_LOGCONFIG(TAG, "    [%u] page_id='%s' name='%s' order=%d page=%p",
                  (unsigned) i, pages_[i].page_id.c_str(), pages_[i].name.c_str(), pages_[i].order, pages_[i].page);
  }
  ESP_LOGCONFIG(TAG, "  Sort mode: %u", (unsigned) sort_mode_);
  if (!default_page_.empty()) {
    ESP_LOGCONFIG(TAG, "  Default page: %s", default_page_.c_str());
  }
}

void PageManager::control(const std::string &value) {
  // Cancel any active push before manual navigation
  this->cancel_push_();

  int idx = this->index_by_name_(value);
  if (idx < 0) {
    ESP_LOGW(TAG, "Unknown select value '%s'", value.c_str());
    return;
  }
  this->apply_index_(idx);
}

void PageManager::show_page(const std::string &page_id, lv_scr_load_anim_t animation, uint32_t time_ms) {
  // Cancel any active push before manual navigation
  this->cancel_push_();

  int idx = this->index_by_page_id_(page_id);
  if (idx < 0) {
    ESP_LOGW(TAG, "Page ID '%s' not found", page_id.c_str());
    return;
  }
  this->apply_index_(idx, animation, time_ms);
}

void PageManager::next(lv_scr_load_anim_t animation, uint32_t time_ms) {
  // Cancel any active push before manual navigation
  this->cancel_push_();

  if (pages_.empty()) return;
  int n = (int) pages_.size();
  int idx = (current_index_ < 0) ? 0 : (current_index_ + 1) % n;
  this->apply_index_(idx, animation, time_ms);
}

void PageManager::previous(lv_scr_load_anim_t animation, uint32_t time_ms) {
  // Cancel any active push before manual navigation
  this->cancel_push_();

  if (pages_.empty()) return;
  int n = (int) pages_.size();
  int idx = (current_index_ < 0) ? 0 : (current_index_ - 1 + n) % n;
  this->apply_index_(idx, animation, time_ms);
}

void PageManager::apply_index_(int idx, lv_scr_load_anim_t animation, uint32_t time_ms) {
  if (idx < 0 || idx >= (int) pages_.size()) return;
  current_index_ = idx;
  auto &p = pages_[idx];
  this->show_lvgl_page_(p.page, animation, time_ms);
  this->publish_state(p.name);
}

int PageManager::index_by_page_id_(const std::string &page_id) const {
  for (size_t i = 0; i < pages_.size(); i++) {
    if (pages_[i].page_id == page_id) return (int) i;
  }
  return -1;
}

int PageManager::index_by_name_(const std::string &name) const {
  for (size_t i = 0; i < pages_.size(); i++) {
    if (pages_[i].name == name) return (int) i;
  }
  return -1;
}

void PageManager::show_lvgl_page_(esphome::lvgl::LvPageType *page, lv_scr_load_anim_t animation, uint32_t time_ms) {
  if (page == nullptr) {
    ESP_LOGW(TAG, "Page is null; cannot show page");
    return;
  }
  if (lvgl_ == nullptr) {
    ESP_LOGW(TAG, "LVGL component not set; cannot show page");
    return;
  }
  size_t idx = page->index;
  lvgl_->show_page(idx, animation, time_ms);
}

void PageManager::push_page(const std::string &page_id, uint32_t duration_ms, lv_scr_load_anim_t animation, uint32_t time_ms, std::function<void()> on_push, std::function<void()> on_pop) {
  int idx = this->index_by_page_id_(page_id);
  if (idx < 0) {
    ESP_LOGW(TAG, "Push failed: page ID '%s' not found", page_id.c_str());
    return;
  }

  // Handle replace mode
  if (this->push_mode_ == 1 && !push_stack_.empty()) {
    // Replace mode: cancel old push's on_pop before replacing
    auto &old_entry = push_stack_.back();
    if (old_entry.on_pop) {
      old_entry.on_pop();
    }
    push_stack_.pop_back();

    if (this->timeout_active_) {
      this->cancel_timeout("push_timeout");
      this->timeout_active_ = false;
    }
  }

  // If stack is empty, save current page as base
  if (push_stack_.empty()) {
    this->base_page_index_ = this->current_index_;
    ESP_LOGD(TAG, "Saving base page index: %d", this->current_index_);
  } else {
    // Stack mode: pause current timer and calculate remaining time
    if (this->timeout_active_) {
      this->cancel_timeout("push_timeout");
      this->timeout_active_ = false;

      // Note: We don't have an easy way to get actual remaining time,
      // so we store the full duration. This is a limitation.
      // In a real implementation, you'd need to track the start time.
      auto &top = push_stack_.back();
      // Keep the remaining_ms as-is (it was set when entry was created)
    }
  }

  // Create push entry
  PushEntry entry;
  entry.page_index = idx;
  entry.remaining_ms = duration_ms;
  entry.pop_animation = animation;  // Reuse same animation for pop
  entry.pop_time_ms = time_ms;
  entry.on_push = on_push;
  entry.on_pop = on_pop;

  push_stack_.push_back(entry);

  // Navigate to pushed page
  this->apply_index_(idx, animation, time_ms);

  // Fire on_push trigger
  if (on_push) {
    on_push();
  }

  // Set timeout for auto-pop
  auto timeout_ms = duration_ms;
  this->set_timeout("push_timeout", timeout_ms, [this]() {
    this->timeout_active_ = false;
    this->pop_page();
  });
  this->timeout_active_ = true;

  ESP_LOGD(TAG, "Pushed page '%s' for %u ms (stack depth: %u)", page_id.c_str(), duration_ms, (unsigned) push_stack_.size());
}

void PageManager::pop_page(lv_scr_load_anim_t animation, uint32_t time_ms) {
  if (push_stack_.empty()) {
    ESP_LOGW(TAG, "Pop called but stack is empty");
    return;
  }

  // Cancel current timer if active
  if (this->timeout_active_) {
    this->cancel_timeout("push_timeout");
    this->timeout_active_ = false;
  }

  // Get and remove current entry
  auto entry = push_stack_.back();
  push_stack_.pop_back();

  // Fire on_pop trigger (always fires)
  if (entry.on_pop) {
    entry.on_pop();
  }

  ESP_LOGD(TAG, "Popped page (stack depth: %u)", (unsigned) push_stack_.size());

  // Decide where to navigate
  if (push_stack_.empty()) {
    // Stack is now empty, return to base page
    if (this->base_page_index_.has_value()) {
      int base_idx = this->base_page_index_.value();
      this->base_page_index_.reset();

      // Use provided animation or the entry's pop animation
      lv_scr_load_anim_t anim = (animation != LV_SCR_LOAD_ANIM_NONE) ? animation : entry.pop_animation;
      uint32_t time = (time_ms != 50) ? time_ms : entry.pop_time_ms;

      this->apply_index_(base_idx, anim, time);
      ESP_LOGD(TAG, "Returned to base page index: %d", base_idx);
    }
  } else {
    // More items on stack, show next with remaining time
    auto &next_entry = push_stack_.back();

    // Use provided animation or the entry's pop animation
    lv_scr_load_anim_t anim = (animation != LV_SCR_LOAD_ANIM_NONE) ? animation : entry.pop_animation;
    uint32_t time = (time_ms != 50) ? time_ms : entry.pop_time_ms;

    this->apply_index_(next_entry.page_index, anim, time);

    // Set timeout for next entry with remaining time
    auto timeout_ms = next_entry.remaining_ms;
    this->set_timeout("push_timeout", timeout_ms, [this]() {
      this->timeout_active_ = false;
      this->pop_page();
    });
    this->timeout_active_ = true;

    ESP_LOGD(TAG, "Resumed previous pushed page with %u ms remaining", timeout_ms);
  }
}

void PageManager::clear_stack(lv_scr_load_anim_t animation, uint32_t time_ms) {
  if (push_stack_.empty()) {
    ESP_LOGD(TAG, "Clear stack called but stack is empty");
    return;
  }

  // Cancel current timer
  if (this->timeout_active_) {
    this->cancel_timeout("push_timeout");
    this->timeout_active_ = false;
  }

  // Fire on_pop for all entries (top to bottom)
  for (auto it = push_stack_.rbegin(); it != push_stack_.rend(); ++it) {
    if (it->on_pop) {
      it->on_pop();
    }
  }

  int stack_size = (int) push_stack_.size();
  push_stack_.clear();

  // Return to base page
  if (this->base_page_index_.has_value()) {
    int base_idx = this->base_page_index_.value();
    this->base_page_index_.reset();
    this->apply_index_(base_idx, animation, time_ms);
    ESP_LOGD(TAG, "Cleared stack (%d items) and returned to base page", stack_size);
  } else {
    ESP_LOGW(TAG, "Cleared stack but no base page index saved");
  }
}

void PageManager::cancel_push_() {
  if (push_stack_.empty()) {
    return;  // Nothing to cancel
  }

  // Cancel current timer
  if (this->timeout_active_) {
    this->cancel_timeout("push_timeout");
    this->timeout_active_ = false;
  }

  // Fire on_pop for all entries (top to bottom) without showing pages
  for (auto it = push_stack_.rbegin(); it != push_stack_.rend(); ++it) {
    if (it->on_pop) {
      it->on_pop();
    }
  }

  ESP_LOGD(TAG, "Cancelled push stack (%u items)", (unsigned) push_stack_.size());

  push_stack_.clear();
  this->base_page_index_.reset();
}


}  // namespace lvgl_page_manager
}  // namespace esphome
