// © Copyright 2025 Stuart Parmenter
// SPDX-License-Identifier: MIT

#pragma once

#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "esphome/components/select/select.h"
#include "esphome/components/button/button.h"
#include "esphome/components/lvgl/lvgl_esphome.h"
#include "esphome/core/automation.h"


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
  void show_page(const std::string &page_id, lv_scr_load_anim_t animation = LV_SCR_LOAD_ANIM_NONE, uint32_t time_ms = 50);
  void next(lv_scr_load_anim_t animation = LV_SCR_LOAD_ANIM_NONE, uint32_t time_ms = 50);
  void previous(lv_scr_load_anim_t animation = LV_SCR_LOAD_ANIM_NONE, uint32_t time_ms = 50);

 protected:
  void apply_index_(int idx, lv_scr_load_anim_t animation = LV_SCR_LOAD_ANIM_NONE, uint32_t time_ms = 50);
  void show_lvgl_page_(esphome::lvgl::LvPageType *page, lv_scr_load_anim_t animation = LV_SCR_LOAD_ANIM_NONE, uint32_t time_ms = 50);
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

// Actions
class NextPageAction : public Action<>, public Parented<PageManager> {
 public:
  void set_animation(lv_scr_load_anim_t anim) { this->animation_ = anim; }
  void set_time(uint32_t time) { this->time_ = time; }
  void play() override { this->parent_->next(this->animation_, this->time_); }
 protected:
  lv_scr_load_anim_t animation_;
  uint32_t time_;
};

class PrevPageAction : public Action<>, public Parented<PageManager> {
 public:
  void set_animation(lv_scr_load_anim_t anim) { this->animation_ = anim; }
  void set_time(uint32_t time) { this->time_ = time; }
  void play() override { this->parent_->previous(this->animation_, this->time_); }
 protected:
  lv_scr_load_anim_t animation_;
  uint32_t time_;
};

template<typename... Ts> class ShowPageAction : public Action<Ts...>, public Parented<PageManager> {
 public:
  TEMPLATABLE_VALUE(std::string, page)
  void set_animation(lv_scr_load_anim_t anim) { this->animation_ = anim; }
  void set_time(uint32_t time) { this->time_ = time; }

  void play(Ts... x) override {
    auto page_id = this->page_.value(x...);
    this->parent_->show_page(page_id, this->animation_, this->time_);
  }
 protected:
  lv_scr_load_anim_t animation_;
  uint32_t time_;
};

}  // namespace lvgl_page_manager
}  // namespace esphome
