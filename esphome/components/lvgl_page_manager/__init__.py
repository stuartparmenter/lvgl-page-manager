# © Copyright 2025 Stuart Parmenter
# SPDX-License-Identifier: MIT

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import select, button, lvgl
from esphome.components.lvgl.defines import LV_ANIM
from esphome.components.lvgl.lv_validation import lv_milliseconds
from esphome.const import CONF_ID
from esphome import automation

CODEOWNERS = ["@your-gh-handle"]
DEPENDENCIES = ["lvgl"]
AUTO_LOAD = ["select", "button"]

lvpm_ns = cg.esphome_ns.namespace("lvgl_page_manager")
PageManager = lvpm_ns.class_("PageManager", cg.Component, select.Select)
NextButton = lvpm_ns.class_("NextButton", button.Button)
PrevButton = lvpm_ns.class_("PrevButton", button.Button)

LVGLPage = cg.RawExpression("esphome::lvgl::LvPageType")
LvglComponent = lvgl.lvgl_ns.class_("LvglComponent")

CONF_SELECT = "select"
CONF_DEFAULT_PAGE = "default_page"
CONF_SORT = "sort"
CONF_PAGES = "pages"
CONF_PAGE = "page"
CONF_FRIENDLY_NAME = "friendly_name"
CONF_ORDER = "order"
CONF_NEXT_BUTTON = "next_button"
CONF_PREV_BUTTON = "prev_button"

CONF_LVGL = "lvgl"
CONF_ANIMATION = "animation"
CONF_TIME = "time"
CONF_PAGE_MANAGER_ID = "page_manager_id"

SORT_MODES = {"by_order": 0, "by_name": 1, "by_page": 2}

SELECT_SCHEMA = select.select_schema(PageManager)

PAGE_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_PAGE): cv.use_id(LVGLPage),
        cv.Required(CONF_FRIENDLY_NAME): cv.string,
        cv.Optional(CONF_ORDER, default=0): cv.int_,
    }
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.Optional(CONF_ID, default="page_manager"): cv.declare_id(PageManager),

            cv.GenerateID(CONF_LVGL): cv.use_id(LvglComponent),

            cv.Required(CONF_SELECT): SELECT_SCHEMA,
            cv.Optional(CONF_DEFAULT_PAGE, default=""): cv.string,
            cv.Optional(CONF_SORT, default="by_order"): cv.enum(SORT_MODES, lower=True),
            cv.Optional(CONF_PAGES, default=[]): cv.ensure_list(PAGE_SCHEMA),
            cv.Optional(CONF_NEXT_BUTTON): button.button_schema(NextButton),
            cv.Optional(CONF_PREV_BUTTON): button.button_schema(PrevButton),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    # Wire LvglComponent
    lvgl_comp = await cg.get_variable(config[CONF_LVGL])
    cg.add(var.set_lvgl(lvgl_comp))

    # Sort pages for options and registration order
    sort_mode_str = config.get(CONF_SORT, "by_order")
    pages = list(config.get(CONF_PAGES, []))
    if sort_mode_str == "by_order":
        pages.sort(key=lambda p: p.get(CONF_ORDER, 0))
    elif sort_mode_str == "by_name":
        pages.sort(key=lambda p: p.get(CONF_FRIENDLY_NAME, ""))
    else:  # by_page
        pages.sort(key=lambda p: str(p.get(CONF_PAGE, "")))

    # Register pages on the C++ side
    for pg in pages:
        page = await cg.get_variable(pg[CONF_PAGE])
        page_id = str(pg[CONF_PAGE])
        cg.add(var.add_page(page_id, pg[CONF_FRIENDLY_NAME], pg[CONF_ORDER], page))

    # Build options for the select entity
    options = [p[CONF_FRIENDLY_NAME] for p in pages]
    await select.register_select(var, config[CONF_SELECT], options=options)

    # Set default page and sort mode
    cg.add(var.set_sort_mode(SORT_MODES[sort_mode_str]))
    if config.get(CONF_DEFAULT_PAGE):
        cg.add(var.set_default_page(config[CONF_DEFAULT_PAGE]))

    # Optional buttons
    if CONF_NEXT_BUTTON in config:
        next_btn = cg.new_Pvariable(config[CONF_NEXT_BUTTON][CONF_ID])
        await button.register_button(next_btn, config[CONF_NEXT_BUTTON])
        cg.add(next_btn.set_manager(var))
        cg.add(var.set_next_button(next_btn))

    if CONF_PREV_BUTTON in config:
        prev_btn = cg.new_Pvariable(config[CONF_PREV_BUTTON][CONF_ID])
        await button.register_button(prev_btn, config[CONF_PREV_BUTTON])
        cg.add(prev_btn.set_manager(var))
        cg.add(var.set_prev_button(prev_btn))


# Actions
NextPageAction = lvpm_ns.class_("NextPageAction", automation.Action)
PrevPageAction = lvpm_ns.class_("PrevPageAction", automation.Action)
ShowPageAction = lvpm_ns.class_("ShowPageAction", automation.Action)

NEXT_PAGE_ACTION_SCHEMA = cv.Schema(
    {
        cv.Optional(CONF_PAGE_MANAGER_ID, default="page_manager"): cv.use_id(PageManager),
        cv.Optional(CONF_ANIMATION, default="OVER_LEFT"): LV_ANIM.one_of,
        cv.Optional(CONF_TIME, default="50ms"): lv_milliseconds,
    }
)

PREV_PAGE_ACTION_SCHEMA = cv.Schema(
    {
        cv.Optional(CONF_PAGE_MANAGER_ID, default="page_manager"): cv.use_id(PageManager),
        cv.Optional(CONF_ANIMATION, default="OVER_RIGHT"): LV_ANIM.one_of,
        cv.Optional(CONF_TIME, default="50ms"): lv_milliseconds,
    }
)

SHOW_PAGE_ACTION_SCHEMA = cv.Schema(
    {
        cv.Optional(CONF_PAGE_MANAGER_ID, default="page_manager"): cv.use_id(PageManager),
        cv.Required(CONF_PAGE): cv.string,
        cv.Optional(CONF_ANIMATION, default="NONE"): LV_ANIM.one_of,
        cv.Optional(CONF_TIME, default="50ms"): lv_milliseconds,
    }
)

@automation.register_action("lvgl_page_manager.page.next", NextPageAction, NEXT_PAGE_ACTION_SCHEMA)
async def next_page_action_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id)
    await cg.register_parented(var, config[CONF_PAGE_MANAGER_ID])
    if CONF_ANIMATION in config:
        animation = await LV_ANIM.process(config[CONF_ANIMATION])
        cg.add(var.set_animation(animation))
    if CONF_TIME in config:
        time = await cg.templatable(config[CONF_TIME], args, cg.uint32)
        cg.add(var.set_time(time))
    return var

@automation.register_action("lvgl_page_manager.page.previous", PrevPageAction, PREV_PAGE_ACTION_SCHEMA)
async def prev_page_action_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id)
    await cg.register_parented(var, config[CONF_PAGE_MANAGER_ID])
    if CONF_ANIMATION in config:
        animation = await LV_ANIM.process(config[CONF_ANIMATION])
        cg.add(var.set_animation(animation))
    if CONF_TIME in config:
        time = await cg.templatable(config[CONF_TIME], args, cg.uint32)
        cg.add(var.set_time(time))
    return var

@automation.register_action("lvgl_page_manager.page.show", ShowPageAction, SHOW_PAGE_ACTION_SCHEMA)
async def show_page_action_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_PAGE_MANAGER_ID])
    if CONF_PAGE in config:
        template_ = await cg.templatable(config[CONF_PAGE], args, cg.std_string)
        cg.add(var.set_page(template_))
    if CONF_ANIMATION in config:
        animation = await LV_ANIM.process(config[CONF_ANIMATION])
        cg.add(var.set_animation(animation))
    if CONF_TIME in config:
        time = await cg.templatable(config[CONF_TIME], args, cg.uint32)
        cg.add(var.set_time(time))
    return var
