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

struct PushEntry {
  int page_index;
  uint32_t remaining_ms;
  lv_scr_load_anim_t pop_animation;
  uint32_t pop_time_ms;
  std::function<void()> on_push{nullptr};
  std::function<void()> on_pop{nullptr};
};

class PageManager;

template<typename... Ts>
class PushTrigger : public Trigger<Ts...> {
 public:
  explicit PushTrigger(PageManager *parent) : parent_(parent) {}
 protected:
  PageManager *parent_;
};

template<typename... Ts>
class PopTrigger : public Trigger<Ts...> {
 public:
  explicit PopTrigger(PageManager *parent) : parent_(parent) {}
 protected:
  PageManager *parent_;
};

class PageManager : public Component, public select::Select {
 public:
  void set_default_page(const std::string &page_id);
  void set_sort_mode(uint8_t mode);  // 0=by_order, 1=by_name, 2=by_page
  void set_push_mode(uint8_t mode);  // 0=stack, 1=replace
  void set_lvgl(esphome::lvgl::LvglComponent *lvgl);
  void add_page(const std::string &page_id, const std::string &name, int order, esphome::lvgl::LvPageType *page);

  // Buttons
  void set_next_button(button::Button *b) { this->next_btn_ = b; }
  void set_prev_button(button::Button *b) { this->prev_btn_ = b; }

  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::BEFORE_CONNECTION; }

  // Select API
  void control(const std::string &value) override;

  // Convenience methods
  void show_page(const std::string &page_id, lv_scr_load_anim_t animation = LV_SCR_LOAD_ANIM_NONE, uint32_t time_ms = 50);
  void next(lv_scr_load_anim_t animation = LV_SCR_LOAD_ANIM_NONE, uint32_t time_ms = 50);
  void previous(lv_scr_load_anim_t animation = LV_SCR_LOAD_ANIM_NONE, uint32_t time_ms = 50);

  // Push/Pop stack methods
  void push_page(const std::string &page_id, uint32_t duration_ms, lv_scr_load_anim_t animation = LV_SCR_LOAD_ANIM_NONE, uint32_t time_ms = 50, std::function<void()> on_push = nullptr, std::function<void()> on_pop = nullptr);
  void pop_page(lv_scr_load_anim_t animation = LV_SCR_LOAD_ANIM_NONE, uint32_t time_ms = 50);
  void clear_stack(lv_scr_load_anim_t animation = LV_SCR_LOAD_ANIM_NONE, uint32_t time_ms = 50);

 protected:
  void apply_index_(int idx, lv_scr_load_anim_t animation = LV_SCR_LOAD_ANIM_NONE, uint32_t time_ms = 50);
  void show_lvgl_page_(esphome::lvgl::LvPageType *page, lv_scr_load_anim_t animation = LV_SCR_LOAD_ANIM_NONE, uint32_t time_ms = 50);
  int index_by_page_id_(const std::string &page_id) const;
  int index_by_name_(const std::string &name) const;
  void cancel_push_();  // Cancel active push and fire all on_pop triggers

 private:
  esphome::lvgl::LvglComponent *lvgl_{nullptr};
  std::vector<PageEntry> pages_;
  int current_index_{-1};
  uint8_t sort_mode_{0};
  uint8_t push_mode_{0};  // 0=stack, 1=replace
  std::string default_page_;
  button::Button *next_btn_{nullptr};
  button::Button *prev_btn_{nullptr};

  // Push/Pop state
  optional<int> base_page_index_;
  std::vector<PushEntry> push_stack_;
  bool timeout_active_{false};
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
template<typename... Ts> class NextPageAction : public Action<Ts...>, public Parented<PageManager> {
 public:
  void set_animation(lv_scr_load_anim_t anim) { this->animation_ = anim; }
  void set_time(uint32_t time) { this->time_ = time; }
  void play(Ts... x) override { this->parent_->next(this->animation_, this->time_); }
 protected:
  lv_scr_load_anim_t animation_;
  uint32_t time_;
};

template<typename... Ts> class PrevPageAction : public Action<Ts...>, public Parented<PageManager> {
 public:
  void set_animation(lv_scr_load_anim_t anim) { this->animation_ = anim; }
  void set_time(uint32_t time) { this->time_ = time; }
  void play(Ts... x) override { this->parent_->previous(this->animation_, this->time_); }
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

template<typename... Ts> class PushPageAction : public Action<Ts...>, public Parented<PageManager> {
 public:
  TEMPLATABLE_VALUE(std::string, page)
  TEMPLATABLE_VALUE(uint32_t, duration)
  void set_animation(lv_scr_load_anim_t anim) { this->animation_ = anim; }
  void set_time(uint32_t time) { this->time_ = time; }
  void set_on_push_trigger(PushTrigger<Ts...> *trigger) { this->on_push_trigger_ = trigger; }
  void set_on_pop_trigger(PopTrigger<Ts...> *trigger) { this->on_pop_trigger_ = trigger; }

  void play(Ts... x) override {
    auto page_id = this->page_.value(x...);
    auto duration = this->duration_.value(x...);

    std::function<void()> on_push_cb = nullptr;
    if (this->on_push_trigger_) {
      on_push_cb = [this, x...]() { this->on_push_trigger_->trigger(x...); };
    }

    std::function<void()> on_pop_cb = nullptr;
    if (this->on_pop_trigger_) {
      on_pop_cb = [this, x...]() { this->on_pop_trigger_->trigger(x...); };
    }

    this->parent_->push_page(page_id, duration, this->animation_, this->time_, on_push_cb, on_pop_cb);
  }

 protected:
  lv_scr_load_anim_t animation_;
  uint32_t time_;
  PushTrigger<Ts...> *on_push_trigger_{nullptr};
  PopTrigger<Ts...> *on_pop_trigger_{nullptr};
};

template<typename... Ts> class PopPageAction : public Action<Ts...>, public Parented<PageManager> {
 public:
  void set_animation(lv_scr_load_anim_t anim) { this->animation_ = anim; }
  void set_time(uint32_t time) { this->time_ = time; }
  void play(Ts... x) override { this->parent_->pop_page(this->animation_, this->time_); }
 protected:
  lv_scr_load_anim_t animation_;
  uint32_t time_;
};

template<typename... Ts> class ClearStackAction : public Action<Ts...>, public Parented<PageManager> {
 public:
  void set_animation(lv_scr_load_anim_t anim) { this->animation_ = anim; }
  void set_time(uint32_t time) { this->time_ = time; }
  void play(Ts... x) override { this->parent_->clear_stack(this->animation_, this->time_); }
 protected:
  lv_scr_load_anim_t animation_;
  uint32_t time_;
};

}  // namespace lvgl_page_manager
}  // namespace esphome
