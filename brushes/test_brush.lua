r = 10
alpha = 1
blur = 0
outline = 1

function button_press(x, y, pressure, button)
  local x_move
  local y_move
  local paint_alpha
  local i

  for i = 1, 4 do
    x_move = math.random(1, r*2)
    y_move = math.random(1, r*2)
    if math.random(1, 2) == 2 then
      x_move = - x_move
    end
    if math.random(1, 2) == 2 then
      y_move = - y_move
    end
    paint_alpha = math.random(10, 100) * 0.01
    DrawBrush(x + x_move, y + y_move, paint_alpha)
  end
end

function motion_notify(x, y, pressure, motion)
  local x_move
  local y_move
  local paint_alpha
  local i

  for i = 1, 4 do
    x_move = math.random(1, r*2)
    y_move = math.random(1, r*2)
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
  r = AdjustmentGetValue(adjust)
  CirclePatternNew(r, alpha, blur, outline)
end

function brush_alpha_change(adjust)
  alpha = AdjustmentGetValue(adjust) * 0.01
  CirclePatternNew(r, alpha, blur, outline)
end

function brush_blur_change(adjust)
  blur = AdjustmentGetValue(adjust) * 0.01
  CirclePatternNew(r, alpha, blur, outline)
end

function brush_outline_change(adjust)
  outline = AdjustmentGetValue(adjust) * 0.01
  CirclePatternNew(r, alpha, blur, outline)
end

function blend_mode_change(combo)
  SetBrushBlendMode(ComboBoxGetActive(combo))
end

function ui_new()
  local box = VBoxNew(false, 0)
  local scale_adjust = AdjustmentNew(r, 1, 100, 1, 1, 0)
  local scale = SpinScaleNew(scale_adjust, "サイズ", 1)
  SignalConnect(scale_adjust, "value_changed", brush_size_change)
  BoxPackStart(box, scale, false, false, 0)

  scale_adjust = AdjustmentNew(alpha*100, 0, 100, 1, 1, 0)
  scale = SpinScaleNew(scale_adjust, "濃度", 1)
  SignalConnect(scale_adjust, "value_changed", brush_alpha_change)
  BoxPackStart(box, scale, false, false, 0)

  scale_adjust = AdjustmentNew(blur*100, 0, 100, 1, 1, 0)
  scale = SpinScaleNew(scale_adjust, "ボケ足", 1)
  SignalConnect(scale_adjust, "value_changed", brush_blur_change)
  BoxPackStart(box, scale, false, false, 0)

  scale_adjust = AdjustmentNew(outline*100, 0, 100, 1, 1, 0)
  scale = SpinScaleNew(scale_adjust, "輪郭の硬さ", 1)
  SignalConnect(scale_adjust, "value_changed", brush_outline_change)
  BoxPackStart(box, scale, false, false, 0)

  local combo = ComboBoxSelectBlendModeNew()
  ComboBoxSetActive(combo, 0)
  SignalConnect(combo, "changed", blend_mode_change)
  BoxPackStart(box, combo, false, false, 0)

  CirclePatternNew(r, alpha, blur, outline)
  
  return box
end
