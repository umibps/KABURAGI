r = 10
dist = 1
zoom = 0.1
alpha = 1
blur = 0
outline = 1

function button_press(x, y, pressure, button)
  local x_move
  local y_move
  local paint_alpha
  local i

  for i = 1, 4 do
    x_move = math.random(1, r*dist*zoom)
    y_move = math.random(1, r*dist*zoom)
    if math.random(1, 2) == 2 then
      x_move = - x_move
    end
    if math.random(1, 2) == 2 then
      y_move = - y_move
    end
    paint_alpha = math.random(10, 100) * 0.01
    DrawBrush(x + x_move, y + y_move, alpha * paint_alpha)
  end
end

function motion_notify(x, y, pressure, motion)
  local x_move
  local y_move
  local paint_alpha
  local i

  for i = 1, 4 do
    x_move = math.random(1, r*dist*zoom)
    y_move = math.random(1, r*dist*zoom)
    if math.random(1, 2) == 2 then
      x_move = - x_move
    end
    if math.random(1, 2) == 2 then
      y_move = - y_move
    end
    paint_alpha = math.random(10, 100) * 0.01
    DrawBrush(x + x_move, y + y_move, alpha * paint_alpha)
  end
end

function button_release(x, y, pressure, button)
end

function brush_size_change(adjust)
  zoom = AdjustmentGetValue(adjust) * 0.01
  ImagePatternNew("sample.png")
  SetBrushZoom(zoom)
end

function brush_alpha_change(adjust)
  alpha = AdjustmentGetValue(adjust) * 0.01
end

function brush_dist_change(adjust)
  dist = AdjustmentGetValue(adjust)
end

function blend_mode_change(combo)
  SetBrushBlendMode(ComboBoxGetActive(combo))
end

function ui_new()
  local box = VBoxNew(false, 0)
  local scale_adjust = AdjustmentNew(zoom*100, 1, 100, 1, 1, 0)
  local scale = SpinScaleNew(scale_adjust, "サイズ", 1)
  local width
  local height
  SignalConnect(scale_adjust, "value_changed", brush_size_change)
  BoxPackStart(box, scale, false, false, 0)

  scale_adjust = AdjustmentNew(alpha*100, 0, 100, 1, 1, 0)
  scale = SpinScaleNew(scale_adjust, "濃度", 1)
  SignalConnect(scale_adjust, "value_changed", brush_alpha_change)
  BoxPackStart(box, scale, false, false, 0)

  scale_adjust = AdjustmentNew(dist, 0.1, 10, 0.1, 0.1, 0)
  scale = SpinScaleNew(scale_adjust, "間隔", 1)
  SignalConnect(scale_adjust, "value_changed", brush_dist_change)
  BoxPackStart(box, scale, false, false, 0)

  local combo = ComboBoxSelectBlendModeNew()
  ComboBoxSetActive(combo, 0)
  SignalConnect(combo, "changed", blend_mode_change)
  BoxPackStart(box, combo, false, false, 0)

  width, height = ImagePatternNew("sample.png")
  if width > height then
    r = width
  else
    r = height
  end

  SetBrushColorizeMode(PATTERN_MODE_FORE_TO_BACK)
  SetBrushZoom(zoom)

  return box
end
