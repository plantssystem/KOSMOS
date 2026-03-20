-- ============================================================
-- KOSMOS MIDI Randomizer (Complete Stable Version)
-- Author: osamu
-- ============================================================

renoise.app():show_status("KOSMOS Loaded!!")

-- ============================================================
-- Parameters
-- ============================================================
local params = {
  pitch_range   = 12,
  vel_min       = 40,
  vel_max       = 120,
  prob          = 100,
  scale_enabled = false,
  scale         = {0,2,4,5,7,9,11},
  note_length   = 300,
}
local randomizer_enabled = true

-- ============================================================
-- Utility
-- ============================================================
local function clamp(v, lo, hi)
  return math.min(math.max(v, lo), hi)
end

local function quantize_to_scale(note)
  if not params.scale_enabled then return note end
  local base = note % 12
  local octave = math.floor(note / 12)
  local closest = params.scale[1]
  local min_dist = 12

  for _, s in ipairs(params.scale) do
    local d = math.abs(s - base)
    if d < min_dist then
      min_dist = d
      closest = s
    end
  end

  return octave * 12 + closest
end

-- ============================================================
-- MIDI OUT (loopMIDI)
-- ============================================================
local function find_loopmidi()
  for _, name in ipairs(renoise.Midi.available_output_devices()) do
    if name:lower():find("loop") then
      return name
    end
  end
  return nil
end

local loop_name = find_loopmidi()
local midi_out = nil

if loop_name then
  midi_out = renoise.Midi.create_output_device(loop_name)
  renoise.app():show_status("MIDI OUT: " .. loop_name)
else
  renoise.app():show_status("No loopMIDI found!")
end

-- ============================================================
-- 完全1声化：今鳴っている全ノートを管理
-- ============================================================
local active_notes = {}

local function stop_all_notes()
  for note, _ in pairs(active_notes) do
    midi_out:send { 0x80, note, 0 }
  end
  active_notes = {}
end

-- ============================================================
-- NoteOff Queue
-- ============================================================
local pending_off = {}

local function process_noteoff()
  local now = os.clock()
  for i = #pending_off, 1, -1 do
    local ev = pending_off[i]
    if ev.time <= now then
      midi_out:send { 0x80, ev.note, 0 }
      active_notes[ev.note] = nil
      table.remove(pending_off, i)
    end
  end
end

renoise.tool():add_timer(process_noteoff, 10)

-- ============================================================
-- MIDI Callback
-- ============================================================
-- ★ 必須：midi_callback より前に置く
local active_notes = {}             -- 完全1声化のためのノート管理

local function stop_all_notes()
  for note, _ in pairs(active_notes) do
    midi_out:send { 0x80, note, 0 }
  end
  active_notes = {}
end

local function midi_callback(message)
  if message.type ~= renoise.Midi.MESSAGE_NOTE_ON then
    return
  end

  local note = message.byte2 or message[2]
  local vel  = message.byte3 or message[3]
  if not note then return end

  ------------------------------------------------------------
  -- ★ Randomizer OFF：原音をそのまま鳴らす
  ------------------------------------------------------------
  if not randomizer_enabled then
    stop_all_notes()
    pending_off = {}

    midi_out:send { 0x90, note, vel }
    active_notes[note] = true

    table.insert(pending_off, {
      time = os.clock() + (params.note_length / 1000),
      note = note
    })

    renoise.app():show_status("RAW:"..note)
    return
  end

  ------------------------------------------------------------
  -- ★ 休符（prob% で鳴らさない）
  ------------------------------------------------------------
  if math.random(0,100) > params.prob then
    stop_all_notes()
    pending_off = {}
    renoise.app():show_status("REST")
    return
  end

  ------------------------------------------------------------
  -- ★ Pitch Randomize + Jitter + Scale
  ------------------------------------------------------------
  -- 大きなランダム
  local new_note = note + math.random(-params.pitch_range, params.pitch_range)

  -- 微細な揺れ（Jitter）
  if params.jitter and params.jitter > 0 then
    new_note = new_note + math.random(-params.jitter, params.jitter)
  end

  new_note = clamp(new_note, 0, 119)

  -- スケール量子化
  new_note = quantize_to_scale(new_note)

  ------------------------------------------------------------
  -- ★ Velocity Randomize
  ------------------------------------------------------------
  local new_vel = clamp(vel, params.vel_min, params.vel_max)

  ------------------------------------------------------------
  -- ★ 完全1声化
  ------------------------------------------------------------
  stop_all_notes()

  midi_out:send { 0x90, new_note, new_vel }
  active_notes[new_note] = true

  ------------------------------------------------------------
  -- ★ 遅延 NoteOff（300ms）
  ------------------------------------------------------------
  table.insert(pending_off, {
    time = os.clock() + (params.note_length / 1000),
    note = new_note
  })

  renoise.app():show_status("IN:"..note.." → OUT:"..new_note)
end

-- ============================================================
-- Device Selection (Pico)
-- ============================================================
local function find_pico()
  for _, name in ipairs(renoise.Midi.available_input_devices()) do
    if name:lower():find("pico") then
      return name
    end
  end
  return nil
end

local pico_name = find_pico()

if pico_name then
  renoise.app():show_status("FOUND PICO: " .. pico_name)
  device = renoise.Midi.create_input_device(pico_name, midi_callback)

  if device == nil then
    renoise.app():show_status("ERROR: Could not open device: " .. pico_name)
  else
    renoise.app():show_status("CONNECTED: " .. pico_name)
  end
else
  renoise.app():show_status("Pico not found!")
end

-- ============================================================
-- GUI
-- ============================================================
local vb = renoise.ViewBuilder()
local dialog = nil

local function show_gui()
  if dialog and dialog.visible then dialog:close() end

  local content = vb:column {
    margin = 10,
    spacing = 8,

    vb:text { text = "KOSMOS MIDI Randomizer", font = "bold" },

    ------------------------------------------------------------
    -- Randomizer ON/OFF
    ------------------------------------------------------------
    vb:button {
      text = randomizer_enabled and "Randomizer: ON" or "Randomizer: OFF",
      notifier = function()
        randomizer_enabled = not randomizer_enabled
        dialog:close()
        show_gui()
      end
    },

    vb:space { height = 10 },

    ------------------------------------------------------------
    -- Pitch Range
    ------------------------------------------------------------
    (function()
      local label = vb:text { text = tostring(params.pitch_range) }
      return vb:row {
        vb:text { text = "Pitch Range (±): " },
        vb:slider {
          min = 0, max = 24, value = params.pitch_range,
          notifier = function(v)
            params.pitch_range = math.floor(v)
            label.text = tostring(params.pitch_range)
          end
        },
        label
      }
    end)(),

    ------------------------------------------------------------
    -- Jitter（揺れ）
    ------------------------------------------------------------
    (function()
      local label = vb:text { text = tostring(params.jitter or 0) }
      return vb:row {
        vb:text { text = "Jitter (±): " },
        vb:slider {
          min = 0, max = 5,
          value = params.jitter or 0,
          notifier = function(v)
            params.jitter = math.floor(v)
            label.text = tostring(params.jitter)
          end
        },
        label
      }
    end)(),

    ------------------------------------------------------------
    -- Velocity Min
    ------------------------------------------------------------
    (function()
      local label = vb:text { text = tostring(params.vel_min) }
      return vb:row {
        vb:text { text = "Velocity Min: " },
        vb:slider {
          min = 1, max = 127, value = params.vel_min,
          notifier = function(v)
            params.vel_min = math.floor(v)
            label.text = tostring(params.vel_min)
          end
        },
        label
      }
    end)(),

    ------------------------------------------------------------
    -- Velocity Max
    ------------------------------------------------------------
    (function()
      local label = vb:text { text = tostring(params.vel_max) }
      return vb:row {
        vb:text { text = "Velocity Max: " },
        vb:slider {
          min = 1, max = 127, value = params.vel_max,
          notifier = function(v)
            params.vel_max = math.floor(v)
            label.text = tostring(params.vel_max)
          end
        },
        label
      }
    end)(),

    ------------------------------------------------------------
    -- Note Length
    ------------------------------------------------------------
    (function()
      local label = vb:text { text = tostring(params.note_length) }
      return vb:row {
        vb:text { text = "Note Length (ms): " },
        vb:slider {
          min = 20, max = 400, value = params.note_length,
          notifier = function(v)
            params.note_length = math.floor(v)
            label.text = tostring(params.note_length)
          end
        },
        label
      }
    end)(),

    ------------------------------------------------------------
    -- 休符（prob）
    ------------------------------------------------------------
    (function()
      local label = vb:text { text = tostring(params.prob) .. "%" }
      return vb:row {
        vb:text { text = "Note Probability (%): " },
        vb:slider {
          min = 0, max = 100, value = params.prob,
          notifier = function(v)
            params.prob = math.floor(v)
            label.text = tostring(params.prob) .. "%"
          end
        },
        label
      }
    end)(),

    ------------------------------------------------------------
    -- Scale Enable
    ------------------------------------------------------------
    vb:row {
      vb:checkbox {
        value = params.scale_enabled,
        notifier = function(v) params.scale_enabled = v end
      },
      vb:text { text = "Enable Scale Quantize" }
    },

    ------------------------------------------------------------
    -- Scale Type
    ------------------------------------------------------------
    vb:row {
      vb:text { text = "Scale: " },
      vb:popup {
        items = { "Major", "Minor", "Hyojo (平調子)", "In (陰音階)", "Yo (陽音階)", "Ryukyu (琉球音階)" },
        notifier = function(idx)
          if idx == 1 then
            params.scale = {0,2,4,5,7,9,11}
          elseif idx == 2 then
            params.scale = {0,2,3,5,7,8,10}
          elseif idx == 3 then
            params.scale = {0,2,4,7,9} -- 平調子
          elseif idx == 4 then
            params.scale = {0,1,5,7,8} -- 陰音階
          elseif idx == 5 then
            params.scale = {0,2,5,7,9} -- 陽音階
          elseif idx == 6 then
            params.scale = {0,4,5,7,11} -- 琉球音階
          end
        end
      }
    },
  }

  dialog = renoise.app():show_custom_dialog("KOSMOS", content)
end

-- ★ メニューに名前を入れる（ここを好きな名前にできる）
renoise.tool():add_menu_entry {
  name = "Main Menu:Tools:KOSMOS MIDI Randomizer",
  invoke = show_gui
}

