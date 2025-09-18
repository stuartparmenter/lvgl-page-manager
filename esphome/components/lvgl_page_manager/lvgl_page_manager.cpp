// © Copyright 2025 Stuart Parmenter
// SPDX-License-Identifier: MIT

#include "lvgl_page_manager.h"
#include <algorithm>

namespace esphome {
namespace lvgl_page_manager {

void PageManager::set_default_page(const std::string &page_id) { this->default_page_ = page_id; }
void PageManager::set_sort_mode(uint8_t mode) { this->sort_mode_ = mode; }
void PageManager::set_lvgl(esphome::lvgl::LvglComponent *lvgl) { this->lvgl_ = lvgl; }
void PageManager::add_page(const std::string &page_id, const std::string &name, int order, esphome::lvgl::LvPageType *page) {
  PageEntry e{page_id, name, order, page};
  pages_.push_back(e);
  option_names_.push_back(name);
  this->traits.set_options(option_names_);
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
  int idx = this->index_by_name_(value);
  if (idx < 0) {
    ESP_LOGW(TAG, "Unknown select value '%s'", value.c_str());
    return;
  }
  this->apply_index_(idx);
}

void PageManager::show_page(const std::string &page_id) {
  int idx = this->index_by_page_id_(page_id);
  if (idx < 0) {
    ESP_LOGW(TAG, "Page ID '%s' not found", page_id.c_str());
    return;
  }
  this->apply_index_(idx);
}

void PageManager::next() {
  if (pages_.empty()) return;
  int n = (int) pages_.size();
  int idx = (current_index_ < 0) ? 0 : (current_index_ + 1) % n;
  this->apply_index_(idx);
}

void PageManager::previous() {
  if (pages_.empty()) return;
  int n = (int) pages_.size();
  int idx = (current_index_ < 0) ? 0 : (current_index_ - 1 + n) % n;
  this->apply_index_(idx);
}

void PageManager::apply_index_(int idx) {
  if (idx < 0 || idx >= (int) pages_.size()) return;
  current_index_ = idx;
  auto &p = pages_[idx];
  this->show_lvgl_page_(p.page);
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

void PageManager::show_lvgl_page_(esphome::lvgl::LvPageType *page) {
  if (page == nullptr) {
    ESP_LOGW(TAG, "Page is null; cannot show page");
    return;
  }
  if (lvgl_ == nullptr) {
    ESP_LOGW(TAG, "LVGL component not set; cannot show page");
    return;
  }
  size_t idx = page->index;
  lvgl_->show_page(idx, LV_SCR_LOAD_ANIM_NONE, 0);
}


}  // namespace lvgl_page_manager
}  // namespace esphome
