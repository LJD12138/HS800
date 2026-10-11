/**
 * EEZ Studio Engineering Mode UI Generator
 * 根据 engineering_mode_spec.md 规范生成 Main_Eng 工程模式界面
 */
const fs = require('fs');
const path = require('path');
const crypto = require('crypto');

const PROJECT_PATH = path.join(
    'G:', '1-Baiku_Projects', '25-HS800', '1.software', 'HS800',
    'APP', 'Hardware', 'MD_Display', 'eez_project', 'eez_project.eez-project'
);

// UUID generator
function uuid() {
    return crypto.randomUUID();
}

// ============================================================
// 1. Variable Definitions
// ============================================================

function makeVar(name, type, defaultValue) {
    return {
        objID: uuid(),
        name: name,
        type: type,
        size: null,
        defaultValue: defaultValue,
        persistent: false,
        native: false
    };
}

function addVariables(project) {
    const vars = project.variables.globalVariables;

    // Navigation control variables
    const navVars = [
        makeVar('uc_eng_page', 'integer', '0'),    // 0=main, 1=param_view, 2=param_set, 3=sys_set
        makeVar('uc_eng_tab', 'integer', '0'),      // current tab index
        makeVar('uc_eng_item', 'integer', '0'),     // current item index within tab
        makeVar('uc_eng_selected', 'integer', '0'), // selected menu item (main menu)
        makeVar('uc_eng_confirm', 'integer', '0'),  // confirm dialog state
    ];

    // Param View variables (spec section 6.1)
    const pvVars = [
        makeVar('uca_pv_bms_volt_curr', 'string', '""'),
        makeVar('uca_pv_bms_soc_cycle', 'string', '""'),
        makeVar('uca_pv_bms_temp', 'string', '""'),
        makeVar('uca_pv_bms_cap_state', 'string', '""'),
        makeVar('uca_pv_bms_online_err', 'string', '""'),
        makeVar('uca_pv_bms_time', 'string', '""'),
        makeVar('uca_pv_bms_perm_pwr', 'string', '""'),
        makeVar('uca_pv_mppt_pv_v_i', 'string', '""'),
        makeVar('uca_pv_mppt_pwr', 'string', '""'),
        makeVar('uca_pv_mppt_out_v_i', 'string', '""'),
        makeVar('uca_pv_mppt_max_temp', 'string', '""'),
        makeVar('uca_pv_mppt_type_err', 'string', '""'),
        makeVar('uca_pv_dcac_in_v_i', 'string', '""'),
        makeVar('uca_pv_dcac_in_p_f', 'string', '""'),
        makeVar('uca_pv_dcac_out_v_i', 'string', '""'),
        makeVar('uca_pv_dcac_out_p_f', 'string', '""'),
        makeVar('uca_pv_dcac_chg_max', 'string', '""'),
        makeVar('uca_pv_dcac_temp', 'string', '""'),
        makeVar('uca_pv_dcac_state', 'string', '""'),
        makeVar('uca_pv_dcac_err', 'string', '""'),
        makeVar('uca_pv_usb_v_i', 'string', '""'),
        makeVar('uca_pv_usb_pwr', 'string', '""'),
        makeVar('uca_pv_usb_temp_err', 'string', '""'),
        makeVar('uca_pv_dc_in_v_i', 'string', '""'),
        makeVar('uca_pv_dc_out_v_i', 'string', '""'),
        makeVar('uca_pv_dc_pwr_err', 'string', '""'),
        makeVar('uca_pv_adc_sys_key', 'string', '""'),
        makeVar('uca_pv_adc_dc_temp_curr', 'string', '""'),
        makeVar('uca_pv_adc_dc_vout', 'string', '""'),
        makeVar('uca_pv_adc_dc_vin', 'string', '""'),
        makeVar('uca_pv_sys_uptime', 'string', '""'),
        makeVar('uca_pv_sys_ver', 'string', '""'),
        makeVar('uca_pv_sys_build', 'string', '""'),
        makeVar('uca_pv_sys_temp', 'string', '""'),
        makeVar('uca_pv_sys_volt_fan', 'string', '""'),
    ];

    // Param Set variables (spec section 6.2)
    const psVars = [
        makeVar('uca_ps_tab_title', 'string', '""'),
        makeVar('uca_ps_param_label', 'string', '""'),
        makeVar('uca_ps_param_value', 'string', '""'),
        makeVar('uca_ps_param_list', 'string', '""'),
    ];

    const existingNames = new Set(vars.map(v => v.name));
    const allNew = [...navVars, ...pvVars, ...psVars];
    const newVars = allNew.filter(v => !existingNames.has(v.name));
    vars.push(...newVars);
}

// ============================================================
// 2. Widget Helper Functions
// ============================================================

const DEFAULT_FLAGS = "CLICK_FOCUSABLE|GESTURE_BUBBLE|PRESS_LOCK|SCROLLABLE|SCROLL_CHAIN_HOR|SCROLL_CHAIN_VER|SCROLL_ELASTIC|SCROLL_MOMENTUM|SCROLL_WITH_ARROW|SNAPPABLE";

function makeLocalStyles(styleDef) {
    return {
        objID: uuid(),
        definition: styleDef || {}
    };
}

function makeBaseWidget(type, left, top, width, height, opts = {}) {
    return {
        objID: uuid(),
        type: type,
        left: left,
        top: top,
        width: width,
        height: height,
        customInputs: [],
        customOutputs: [],
        style: opts.style || {
            objID: uuid(),
            useStyle: "default",
            conditionalStyles: [],
            childStyles: []
        },
        locked: false,
        timeline: [],
        eventHandlers: [],
        leftUnit: opts.leftUnit || "px",
        topUnit: opts.topUnit || "px",
        widthUnit: opts.widthUnit || "px",
        heightUnit: opts.heightUnit || "px",
        children: opts.children || [],
        widgetFlags: DEFAULT_FLAGS,
        hiddenFlagType: opts.hiddenFlagType || "literal",
        clickableFlagType: "literal",
        flagScrollbarMode: "",
        flagScrollDirection: "",
        scrollSnapX: "",
        scrollSnapY: "",
        checkedStateType: "literal",
        disabledStateType: "literal",
        states: "",
        localStyles: opts.localStyles || makeLocalStyles(),
        group: "",
        groupIndex: 0,
    };
}

function makeLabel(left, top, width, height, text, opts = {}) {
    const w = makeBaseWidget("LVGLLabelWidget", left, top, width, height, {
        style: opts.style,
        widthUnit: opts.widthUnit || "content",
        heightUnit: opts.heightUnit || "content",
        localStyles: opts.localStyles || makeLocalStyles(),
        hiddenFlagType: opts.hiddenFlagType || "literal",
    });
    w.text = text;
    w.textType = opts.textType || "literal";
    w.longMode = opts.longMode || "WRAP";
    w.recolor = opts.recolor || false;
    w.previewValue = opts.previewValue || text;
    if (opts.identifier) w.identifier = opts.identifier;
    return w;
}

function makePanel(left, top, width, height, opts = {}) {
    const w = makeBaseWidget("LVGLPanelWidget", left, top, width, height, {
        style: opts.style,
        localStyles: opts.localStyles || makeLocalStyles(),
        children: opts.children || [],
        hiddenFlagType: opts.hiddenFlagType || "literal",
    });
    return w;
}

function makeButton(left, top, width, height, label, opts = {}) {
    const btnChildren = [];
    if (label) {
        const lbl = makeLabel(0, 0, width, height, label, {
            widthUnit: "percent",
            heightUnit: "percent",
            localStyles: makeLocalStyles({
                MAIN: {
                    DEFAULT: {
                        text_font: opts.font || undefined,
                        text_color: opts.textColor || "#FFFFFF",
                        text_align: "CENTER",
                        align: "CENTER"
                    }
                }
            })
        });
        lbl.left = 0;
        lbl.top = 0;
        btnChildren.push(lbl);
    }

    const w = makeBaseWidget("LVGLButtonWidget", left, top, width, height, {
        style: opts.style,
        localStyles: opts.localStyles || makeLocalStyles(),
        children: btnChildren,
        hiddenFlagType: opts.hiddenFlagType || "literal",
    });
    if (opts.identifier) w.identifier = opts.identifier;
    return w;
}

// ============================================================
// 3. Color Constants
// ============================================================

const COLORS = {
    BG: "#0F131A",
    CARD: "#1A202C",
    BORDER: "#2D3748",
    BMS: "#00E676",
    MPPT: "#00E5FF",
    DCAC: "#FFB300",
    SYS: "#718096",
    USB: "#9C27B0",
    DC: "#FF5722",
    ADC: "#E91E63",
    TEXT: "#FFFFFF",
    TEXT_SECONDARY: "#718096",
    HIGHLIGHT: "#00E5FF",
    MENU_BG: "#1E293B",
    MENU_SELECTED: "#0D2847",
};

// ============================================================
// 4. Page Visibility Control
// ============================================================

// Creates a conditional style that hides widget when uc_eng_page != targetPage
function pageVisibilityStyle(targetPage) {
    return {
        objID: uuid(),
        useStyle: "default",
        conditionalStyles: [
            {
                objID: uuid(),
                condition: {
                    objID: uuid(),
                    value: `${targetPage}`,
                    variable: "uc_eng_page",
                    op: "!="
                },
                style: {
                    objID: uuid(),
                    definition: {
                        MAIN: {
                            DEFAULT: {
                                hidden: true
                            }
                        }
                    }
                }
            }
        ],
        childStyles: []
    };
}

// Tab visibility conditional style
function tabVisibilityStyle(targetTab) {
    return {
        objID: uuid(),
        useStyle: "default",
        conditionalStyles: [
            {
                objID: uuid(),
                condition: {
                    objID: uuid(),
                    value: `${targetTab}`,
                    variable: "uc_eng_tab",
                    op: "!="
                },
                style: {
                    objID: uuid(),
                    definition: {
                        MAIN: {
                            DEFAULT: {
                                hidden: true
                            }
                        }
                    }
                }
            }
        ],
        childStyles: []
    };
}

// Combined page+tab visibility
function pageTabVisibilityStyle(targetPage, targetTab) {
    return {
        objID: uuid(),
        useStyle: "default",
        conditionalStyles: [
            {
                objID: uuid(),
                condition: {
                    objID: uuid(),
                    value: `${targetPage}`,
                    variable: "uc_eng_page",
                    op: "!="
                },
                style: {
                    objID: uuid(),
                    definition: {
                        MAIN: {
                            DEFAULT: {
                                hidden: true
                            }
                        }
                    }
                }
            },
            {
                objID: uuid(),
                condition: {
                    objID: uuid(),
                    value: `${targetTab}`,
                    variable: "uc_eng_tab",
                    op: "!="
                },
                style: {
                    objID: uuid(),
                    definition: {
                        MAIN: {
                            DEFAULT: {
                                hidden: true
                            }
                        }
                    }
                }
            }
        ],
        childStyles: []
    };
}

// ============================================================
// 5. Build Main Menu (Page 0)
// ============================================================

function buildMainMenu() {
    const children = [];

    // Title bar background
    children.push(makePanel(0, 0, 320, 24, {
        localStyles: makeLocalStyles({
            MAIN: {
                DEFAULT: {
                    bg_color: COLORS.CARD,
                    border_side: "BOTTOM",
                    border_color: COLORS.BORDER,
                    border_width: 1
                }
            }
        })
    }));

    // Title "ENG MODE"
    children.push(makeLabel(8, 0, 120, 24, "ENG MODE", {
        localStyles: makeLocalStyles({
            MAIN: {
                DEFAULT: {
                    text_font: "BarlowCondensed-Regular-26",
                    text_color: COLORS.TEXT,
                    align: "CENTER"
                }
            }
        })
    }));

    // EXIT button
    children.push(makeButton(262, 3, 50, 18, "EXIT", {
        localStyles: makeLocalStyles({
            MAIN: {
                DEFAULT: {
                    bg_color: COLORS.BORDER,
                    bg_opa: 180,
                    radius: 3
                }
            }
        })
    }));

    // Menu items
    const menuItems = [
        { title: "PARAM VIEW", subtitle: "Realtime telemetry", color: COLORS.BMS, idx: 0 },
        { title: "PARAM SET", subtitle: "Memory param config", color: COLORS.DCAC, idx: 1 },
        { title: "SYS SET", subtitle: "Save / Reset / Update", color: COLORS.HIGHLIGHT, idx: 2 },
    ];

    menuItems.forEach((item, i) => {
        const y = 32 + i * 56;

        // Menu item background panel
        const menuPanel = makePanel(16, y, 288, 48, {
            localStyles: makeLocalStyles({
                MAIN: {
                    DEFAULT: {
                        bg_color: COLORS.MENU_BG,
                        radius: 6,
                        border_side: "LEFT",
                        border_color: item.color,
                        border_width: 3,
                        pad_left: 10
                    }
                }
            }),
            children: [
                // Title
                makeLabel(14, 6, 200, 22, item.title, {
                    localStyles: makeLocalStyles({
                        MAIN: {
                            DEFAULT: {
                                text_font: "BarlowCondensed-Regular-26",
                                text_color: item.color
                            }
                        }
                    })
                }),
                // Subtitle
                makeLabel(14, 28, 200, 16, item.subtitle, {
                    localStyles: makeLocalStyles({
                        MAIN: {
                            DEFAULT: {
                                text_color: COLORS.TEXT_SECONDARY
                            }
                        }
                    })
                }),
            ]
        });
        children.push(menuPanel);
    });

    return children;
}

// ============================================================
// 6. Build Param View Page (Page 1)
// ============================================================

function buildParamViewHeader(tabTitle, accentColor) {
    const children = [];

    // Header bar
    children.push(makePanel(0, 0, 320, 24, {
        localStyles: makeLocalStyles({
            MAIN: {
                DEFAULT: {
                    bg_color: COLORS.CARD,
                    border_side: "BOTTOM",
                    border_color: COLORS.BORDER,
                    border_width: 1
                }
            }
        })
    }));

    // Back button
    children.push(makeButton(4, 3, 50, 18, "< BACK", {
        localStyles: makeLocalStyles({
            MAIN: {
                DEFAULT: {
                    bg_color: COLORS.BORDER,
                    bg_opa: 180,
                    radius: 3
                }
            }
        })
    }));

    // Tab title
    children.push(makeLabel(62, 1, 180, 22, tabTitle, {
        localStyles: makeLocalStyles({
            MAIN: {
                DEFAULT: {
                    text_font: "BarlowCondensed-Regular-26",
                    text_color: accentColor,
                    text_align: "CENTER",
                    align: "CENTER"
                }
            }
        })
    }));

    return children;
}

function buildParamViewTabBar() {
    const children = [];
    const tabs = ["BMS", "MPPT", "DCAC", "USB", "DC", "ADC", "SYS"];
    const colors = [COLORS.BMS, COLORS.MPPT, COLORS.DCAC, COLORS.USB, COLORS.DC, COLORS.ADC, COLORS.SYS];
    const tabW = 42;
    const startX = 13;

    // Tab bar background
    children.push(makePanel(0, 224, 320, 16, {
        localStyles: makeLocalStyles({
            MAIN: {
                DEFAULT: {
                    bg_color: COLORS.CARD,
                    border_side: "TOP",
                    border_color: COLORS.BORDER,
                    border_width: 1
                }
            }
        })
    }));

    tabs.forEach((tab, i) => {
        const lbl = makeLabel(startX + i * tabW, 225, tabW, 14, tab, {
            localStyles: makeLocalStyles({
                MAIN: {
                    DEFAULT: {
                        text_color: colors[i],
                        text_align: "CENTER",
                        align: "CENTER"
                    }
                }
            })
        });
        children.push(lbl);
    });

    return children;
}

function makeDataRow(y, leftLabel, leftVar, rightLabel, rightVar, accentColor) {
    const children = [];
    const rowH = 22;
    const leftX = 8;
    const rightX = 160;

    if (leftLabel) {
        children.push(makeLabel(leftX, y, 60, rowH, leftLabel, {
            localStyles: makeLocalStyles({
                MAIN: { DEFAULT: { text_color: COLORS.TEXT_SECONDARY } }
            })
        }));
    }
    if (leftVar) {
        children.push(makeLabel(leftX + 62, y, 86, rowH, leftVar, {
            textType: "expression",
            localStyles: makeLocalStyles({
                MAIN: { DEFAULT: { text_color: accentColor || COLORS.TEXT } }
            })
        }));
    }
    if (rightLabel) {
        children.push(makeLabel(rightX, y, 60, rowH, rightLabel, {
            localStyles: makeLocalStyles({
                MAIN: { DEFAULT: { text_color: COLORS.TEXT_SECONDARY } }
            })
        }));
    }
    if (rightVar) {
        children.push(makeLabel(rightX + 62, y, 86, rowH, rightVar, {
            textType: "expression",
            localStyles: makeLocalStyles({
                MAIN: { DEFAULT: { text_color: accentColor || COLORS.TEXT } }
            })
        }));
    }

    return children;
}

function buildParamViewContent() {
    const children = [];
    const startY = 28;

    // ---- BMS Tab (tab=0) ----
    const bmsPanel = makePanel(0, startY, 320, 196, {
        style: pageTabVisibilityStyle(1, 0),
        localStyles: makeLocalStyles({ MAIN: { DEFAULT: { bg_color: COLORS.BG, bg_opa: 0 } } }),
        children: [
            ...makeDataRow(4, "V_pack", "uca_pv_bms_volt_curr", "I_pack", null, COLORS.BMS),
            ...makeDataRow(26, "SOC", "uca_pv_bms_soc_cycle", "Cycle", null, COLORS.BMS),
            ...makeDataRow(48, "T_max", "uca_pv_bms_temp", "T_min", null, COLORS.BMS),
            ...makeDataRow(70, "Cap", "uca_pv_bms_cap_state", "T_board", null, COLORS.BMS),
            ...makeDataRow(92, "Online", "uca_pv_bms_online_err", "State", null, COLORS.BMS),
            ...makeDataRow(114, "ChgFull", "uca_pv_bms_time", "DisEmpty", null, COLORS.BMS),
            ...makeDataRow(136, "PermPwr", "uca_pv_bms_perm_pwr", "ErrCode", null, COLORS.BMS),
        ]
    });
    children.push(bmsPanel);

    // ---- MPPT Tab (tab=1) ----
    const mpptPanel = makePanel(0, startY, 320, 196, {
        style: pageTabVisibilityStyle(1, 1),
        localStyles: makeLocalStyles({ MAIN: { DEFAULT: { bg_color: COLORS.BG, bg_opa: 0 } } }),
        children: [
            ...makeDataRow(4, "V_pv_in", "uca_pv_mppt_pv_v_i", "I_pv_in", null, COLORS.MPPT),
            ...makeDataRow(26, "P_pv_in", "uca_pv_mppt_pwr", "P_out", null, COLORS.MPPT),
            ...makeDataRow(48, "V_out", "uca_pv_mppt_out_v_i", "I_out", null, COLORS.MPPT),
            ...makeDataRow(70, "P_max", "uca_pv_mppt_max_temp", "Temp", null, COLORS.MPPT),
            ...makeDataRow(92, "InType", "uca_pv_mppt_type_err", "ErrCode", null, COLORS.MPPT),
        ]
    });
    children.push(mpptPanel);

    // ---- DCAC Tab (tab=2) ----
    const dcacPanel = makePanel(0, startY, 320, 196, {
        style: pageTabVisibilityStyle(1, 2),
        localStyles: makeLocalStyles({ MAIN: { DEFAULT: { bg_color: COLORS.BG, bg_opa: 0 } } }),
        children: [
            ...makeDataRow(4, "V_ac_in", "uca_pv_dcac_in_v_i", "I_ac_in", null, COLORS.DCAC),
            ...makeDataRow(26, "P_ac_in", "uca_pv_dcac_in_p_f", "F_ac_in", null, COLORS.DCAC),
            ...makeDataRow(48, "V_ac_out", "uca_pv_dcac_out_v_i", "I_ac_out", null, COLORS.DCAC),
            ...makeDataRow(70, "P_ac_out", "uca_pv_dcac_out_p_f", "F_ac_out", null, COLORS.DCAC),
            ...makeDataRow(92, "P_chg", "uca_pv_dcac_chg_max", "P_max_in", null, COLORS.DCAC),
            ...makeDataRow(114, "T_max", "uca_pv_dcac_temp", "T_min", null, COLORS.DCAC),
            ...makeDataRow(136, "ChgSt", "uca_pv_dcac_state", "DisChgSt", null, COLORS.DCAC),
            ...makeDataRow(158, "State", "uca_pv_dcac_err", "ErrCode", null, COLORS.DCAC),
        ]
    });
    children.push(dcacPanel);

    // ---- USB Tab (tab=3) ----
    const usbPanel = makePanel(0, startY, 320, 196, {
        style: pageTabVisibilityStyle(1, 3),
        localStyles: makeLocalStyles({ MAIN: { DEFAULT: { bg_color: COLORS.BG, bg_opa: 0 } } }),
        children: [
            ...makeDataRow(4, "V_in", "uca_pv_usb_v_i", "I_in", null, COLORS.USB),
            ...makeDataRow(26, "P_out", "uca_pv_usb_pwr", "P_wc", null, COLORS.USB),
            ...makeDataRow(48, "Temp", "uca_pv_usb_temp_err", "ErrCode", null, COLORS.USB),
        ]
    });
    children.push(usbPanel);

    // ---- DC Tab (tab=4) ----
    const dcPanel = makePanel(0, startY, 320, 196, {
        style: pageTabVisibilityStyle(1, 4),
        localStyles: makeLocalStyles({ MAIN: { DEFAULT: { bg_color: COLORS.BG, bg_opa: 0 } } }),
        children: [
            ...makeDataRow(4, "V_in", "uca_pv_dc_in_v_i", "I_in", null, COLORS.DC),
            ...makeDataRow(26, "V_out", "uca_pv_dc_out_v_i", "I_out", null, COLORS.DC),
            ...makeDataRow(48, "P_out", "uca_pv_dc_pwr_err", "ErrCode", null, COLORS.DC),
        ]
    });
    children.push(dcPanel);

    // ---- ADC Tab (tab=5) ----
    const adcPanel = makePanel(0, startY, 320, 196, {
        style: pageTabVisibilityStyle(1, 5),
        localStyles: makeLocalStyles({ MAIN: { DEFAULT: { bg_color: COLORS.BG, bg_opa: 0 } } }),
        children: [
            ...makeDataRow(4, "V_sys", "uca_pv_adc_sys_key", "KeyPwr", null, COLORS.ADC),
            ...makeDataRow(26, "DC_Tmp", "uca_pv_adc_dc_temp_curr", "DC_Cur", null, COLORS.ADC),
            ...makeDataRow(48, "DC_Vout", "uca_pv_adc_dc_vout", "", null, COLORS.ADC),
            ...makeDataRow(70, "DC_Vin1", "uca_pv_adc_dc_vin", "DC_Vin2", null, COLORS.ADC),
        ]
    });
    children.push(adcPanel);

    // ---- SYS Tab (tab=6) ----
    const sysPanel = makePanel(0, startY, 320, 196, {
        style: pageTabVisibilityStyle(1, 6),
        localStyles: makeLocalStyles({ MAIN: { DEFAULT: { bg_color: COLORS.BG, bg_opa: 0 } } }),
        children: [
            ...makeDataRow(4, "Uptime", "uca_pv_sys_uptime", "ErrCode", null, COLORS.SYS),
            ...makeDataRow(26, "Version", "uca_pv_sys_ver", "", null, COLORS.SYS),
            ...makeDataRow(48, "BuildDt", "uca_pv_sys_build", "", null, COLORS.SYS),
            ...makeDataRow(70, "BrdTmp", "uca_pv_sys_temp", "UsbTmp", null, COLORS.SYS),
            ...makeDataRow(92, "SysVolt", "uca_pv_sys_volt_fan", "FanMode", null, COLORS.SYS),
        ]
    });
    children.push(sysPanel);

    return children;
}

function buildParamViewPage() {
    const children = [];

    // All Param View elements are inside a container panel with page visibility
    const headerChildren = buildParamViewHeader("PARAM VIEW", COLORS.HIGHLIGHT);
    const tabContentChildren = buildParamViewContent();
    const tabBarChildren = buildParamViewTabBar();

    // Header - visible when page=1
    headerChildren.forEach(c => {
        c.style = pageVisibilityStyle(1);
        children.push(c);
    });

    // Content - visible when page=1
    tabContentChildren.forEach(c => {
        // Already has page+tab visibility from makePanel
        // Need to add page=1 visibility as well
        children.push(c);
    });

    // Tab bar - visible when page=1
    tabBarChildren.forEach(c => {
        c.style = pageVisibilityStyle(1);
        children.push(c);
    });

    return children;
}

// ============================================================
// 7. Build Param Set Page (Page 2)
// ============================================================

function buildParamSetPage() {
    const children = [];

    // Header
    const headerEls = [];
    // Header bar
    headerEls.push(makePanel(0, 0, 320, 24, {
        localStyles: makeLocalStyles({
            MAIN: {
                DEFAULT: {
                    bg_color: COLORS.CARD,
                    border_side: "BOTTOM",
                    border_color: COLORS.BORDER,
                    border_width: 1
                }
            }
        })
    }));
    // Back button
    headerEls.push(makeButton(4, 3, 50, 18, "< BACK", {
        localStyles: makeLocalStyles({
            MAIN: { DEFAULT: { bg_color: COLORS.BORDER, bg_opa: 180, radius: 3 } }
        })
    }));
    // Tab title (bound to variable)
    headerEls.push(makeLabel(62, 1, 180, 22, "uca_ps_tab_title", {
        textType: "expression",
        localStyles: makeLocalStyles({
            MAIN: {
                DEFAULT: {
                    text_font: "BarlowCondensed-Regular-26",
                    text_color: COLORS.DCAC,
                    text_align: "CENTER",
                    align: "CENTER"
                }
            }
        })
    }));

    headerEls.forEach(c => {
        c.style = pageVisibilityStyle(2);
        children.push(c);
    });

    // Content area - current parameter display
    const contentPanel = makePanel(8, 28, 304, 186, {
        style: pageVisibilityStyle(2),
        localStyles: makeLocalStyles({
            MAIN: { DEFAULT: { bg_color: COLORS.CARD, radius: 6, pad_all: 8 } }
        }),
        children: [
            // Parameter label
            makeLabel(8, 8, 288, 20, "uca_ps_param_label", {
                textType: "expression",
                localStyles: makeLocalStyles({
                    MAIN: { DEFAULT: { text_color: COLORS.TEXT_SECONDARY } }
                })
            }),
            // Parameter value (large)
            makeLabel(8, 36, 288, 40, "uca_ps_param_value", {
                textType: "expression",
                localStyles: makeLocalStyles({
                    MAIN: {
                        DEFAULT: {
                            text_font: "BarlowCondensed-Regular-26",
                            text_color: COLORS.HIGHLIGHT
                        }
                    }
                })
            }),
            // UP/DOWN hint
            makeLabel(8, 80, 288, 16, "UP/DOWN: adjust", {
                localStyles: makeLocalStyles({
                    MAIN: { DEFAULT: { text_color: COLORS.TEXT_SECONDARY } }
                })
            }),
            // Parameter list
            makeLabel(8, 102, 288, 80, "uca_ps_param_list", {
                textType: "expression",
                localStyles: makeLocalStyles({
                    MAIN: { DEFAULT: { text_color: COLORS.TEXT } }
                })
            }),
        ]
    });
    children.push(contentPanel);

    // Tab bar
    const psTabs = ["SYS", "LCD", "BAT", "MPPT", "DCAC", "USB", "DC"];
    const psColors = [COLORS.SYS, COLORS.HIGHLIGHT, COLORS.BMS, COLORS.MPPT, COLORS.DCAC, COLORS.USB, COLORS.DC];
    const tabW = 42;
    const startX = 13;

    children.push(makePanel(0, 224, 320, 16, {
        style: pageVisibilityStyle(2),
        localStyles: makeLocalStyles({
            MAIN: {
                DEFAULT: {
                    bg_color: COLORS.CARD,
                    border_side: "TOP",
                    border_color: COLORS.BORDER,
                    border_width: 1
                }
            }
        })
    }));

    psTabs.forEach((tab, i) => {
        const lbl = makeLabel(startX + i * tabW, 225, tabW, 14, tab, {
            style: pageVisibilityStyle(2),
            localStyles: makeLocalStyles({
                MAIN: { DEFAULT: { text_color: psColors[i], text_align: "CENTER", align: "CENTER" } }
            })
        });
        children.push(lbl);
    });

    return children;
}

// ============================================================
// 8. Build Sys Set Page (Page 3)
// ============================================================

function buildSysSetPage() {
    const children = [];

    // Header
    const headerEls = [];
    headerEls.push(makePanel(0, 0, 320, 24, {
        localStyles: makeLocalStyles({
            MAIN: {
                DEFAULT: {
                    bg_color: COLORS.CARD,
                    border_side: "BOTTOM",
                    border_color: COLORS.BORDER,
                    border_width: 1
                }
            }
        })
    }));
    headerEls.push(makeButton(4, 3, 50, 18, "< BACK", {
        localStyles: makeLocalStyles({
            MAIN: { DEFAULT: { bg_color: COLORS.BORDER, bg_opa: 180, radius: 3 } }
        })
    }));
    headerEls.push(makeLabel(100, 1, 120, 22, "SYS SET", {
        localStyles: makeLocalStyles({
            MAIN: {
                DEFAULT: {
                    text_font: "BarlowCondensed-Regular-26",
                    text_color: COLORS.HIGHLIGHT,
                    text_align: "CENTER",
                    align: "CENTER"
                }
            }
        })
    }));

    headerEls.forEach(c => {
        c.style = pageVisibilityStyle(3);
        children.push(c);
    });

    // Operation buttons
    const ops = [
        { title: "SAVE & EXIT", desc: "Save & exit eng mode", color: COLORS.BMS, idx: 0 },
        { title: "RESET DEFAULTS", desc: "Reset to defaults", color: COLORS.DCAC, idx: 1 },
        { title: "FIRMWARE UPDATE", desc: "Jump to bootloader", color: COLORS.HIGHLIGHT, idx: 2 },
    ];

    ops.forEach((op, i) => {
        const y = 32 + i * 62;
        const btn = makePanel(16, y, 288, 52, {
            style: pageVisibilityStyle(3),
            localStyles: makeLocalStyles({
                MAIN: {
                    DEFAULT: {
                        bg_color: COLORS.MENU_BG,
                        radius: 6,
                        border_side: "LEFT",
                        border_color: op.color,
                        border_width: 3,
                        pad_left: 10
                    }
                }
            }),
            children: [
                makeLabel(14, 8, 260, 24, op.title, {
                    localStyles: makeLocalStyles({
                        MAIN: { DEFAULT: { text_font: "BarlowCondensed-Regular-26", text_color: op.color } }
                    })
                }),
                makeLabel(14, 32, 260, 16, op.desc, {
                    localStyles: makeLocalStyles({
                        MAIN: { DEFAULT: { text_color: COLORS.TEXT_SECONDARY } }
                    })
                }),
            ]
        });
        children.push(btn);
    });

    return children;
}

// ============================================================
// 9. Build Complete Main_Eng Page
// ============================================================

function buildMainEngPage() {
    const screenChildren = [];

    // Page 0: Main Menu
    const mainMenuChildren = buildMainMenu();
    mainMenuChildren.forEach(c => {
        c.style = pageVisibilityStyle(0);
        screenChildren.push(c);
    });

    // Page 1: Param View
    const pvChildren = buildParamViewPage();
    screenChildren.push(...pvChildren);

    // Page 2: Param Set
    const psChildren = buildParamSetPage();
    screenChildren.push(...psChildren);

    // Page 3: Sys Set
    const ssChildren = buildSysSetPage();
    screenChildren.push(...ssChildren);

    // Screen widget
    const screen = makeBaseWidget("LVGLScreenWidget", 0, 0, 320, 240, {
        localStyles: makeLocalStyles({
            MAIN: {
                DEFAULT: {
                    bg_color: COLORS.BG
                }
            }
        }),
        children: screenChildren,
    });
    // Screen specific fields
    screen.identifier = "scr_eng";
    delete screen.widthUnit;
    delete screen.heightUnit;
    delete screen.leftUnit;
    delete screen.topUnit;

    return {
        objID: uuid(),
        components: [screen],
        connectionLines: [],
        localVariables: [],
        componentGroups: [],
        userProperties: [],
        name: "Main_Eng",
        left: 0,
        top: 0,
        width: 320,
        height: 240,
        createAtStart: true,
        deleteOnScreenUnload: false
    };
}

// ============================================================
// 10. Main
// ============================================================

function main() {
    console.log('Reading project file...');
    const projectStr = fs.readFileSync(PROJECT_PATH, 'utf8');
    const project = JSON.parse(projectStr);

    console.log('Adding variables...');
    addVariables(project);

    console.log('Building Main_Eng page...');
    const newEngPage = buildMainEngPage();

    // Replace existing Main_Eng page
    const engIdx = project.userPages.findIndex(p => p.name === 'Main_Eng');
    if (engIdx >= 0) {
        console.log(`Replacing existing Main_Eng page at index ${engIdx}`);
        project.userPages[engIdx] = newEngPage;
    } else {
        console.log('Adding new Main_Eng page');
        project.userPages.push(newEngPage);
    }

    console.log('Writing project file...');
    const output = JSON.stringify(project, null, 2);
    fs.writeFileSync(PROJECT_PATH, output, 'utf8');

    console.log('Done! Project file updated successfully.');
    console.log(`Total variables: ${project.variables.globalVariables.length}`);
    console.log(`Total pages: ${project.userPages.length}`);
    console.log(`Main_Eng screen children: ${newEngPage.components[0].children.length}`);
}

main();
