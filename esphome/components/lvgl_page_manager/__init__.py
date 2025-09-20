# © Copyright 2025 Stuart Parmenter
# SPDX-License-Identifier: MIT

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import select, button, lvgl
from esphome.const import CONF_ID

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
            cv.GenerateID(): cv.declare_id(PageManager),

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
