// © Copyright 2025 Stuart Parmenter
// SPDX-License-Identifier: MIT

#pragma once

#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "esphome/components/select/select.h"
#include "esphome/components/button/button.h"
#include "esphome/components/lvgl/lvgl_esphome.h"


#include <string>
#include <vector>

namespace esphome {
namespace lvgl_page_manager {

static const char *const TAG = "lvgl_page_manager";

struct PageEntry {
  std::string page_id;
  std::string name;
  int order{0};
  esphome::lvgl::LvPageType *page{nullptr};
};

class PageManager : public Component, public select::Select {
 public:
  void set_default_page(const std::string &page_id);
  void set_sort_mode(uint8_t mode);  // 0=by_order, 1=by_name, 2=by_page
  void set_lvgl(esphome::lvgl::LvglComponent *lvgl);
  void add_page(const std::string &page_id, const std::string &name, int order, esphome::lvgl::LvPageType *page);

  // Buttons
  void set_next_button(button::Button *b) { this->next_btn_ = b; }
  void set_prev_button(button::Button *b) { this->prev_btn_ = b; }

  // Select API
  void setup() override;
  void dump_config() override;
  void control(const std::string &value) override;

  // Convenience methods you already had
  void show_page(const std::string &page_id);
  void next();
  void previous();

 protected:
  void apply_index_(int idx);
  void show_lvgl_page_(esphome::lvgl::LvPageType *page);
  int index_by_page_id_(const std::string &page_id) const;
  int index_by_name_(const std::string &name) const;


 private:
  esphome::lvgl::LvglComponent *lvgl_{nullptr};
  std::vector<PageEntry> pages_;
  std::vector<std::string> option_names_;
  int current_index_{-1};
  uint8_t sort_mode_{0};
  std::string default_page_;
  button::Button *next_btn_{nullptr};
  button::Button *prev_btn_{nullptr};
};

class NextButton : public button::Button {
 public:
  void set_manager(PageManager *pm) { pm_ = pm; }
 protected:
  void press_action() override { if (pm_ != nullptr) pm_->next(); }
  PageManager *pm_{nullptr};
};

class PrevButton : public button::Button {
 public:
  void set_manager(PageManager *pm) { pm_ = pm; }
 protected:
  void press_action() override { if (pm_ != nullptr) pm_->previous(); }
  PageManager *pm_{nullptr};
};

}  // namespace lvgl_page_manager
}  // namespace esphome
