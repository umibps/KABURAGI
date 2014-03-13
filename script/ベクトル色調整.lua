function button_clicked(widget, table)
  local name = ComboBoxGetValue(table.combo_box)
  local blur_value = AdjustmentGetValue(table.blur)
  local hardness_value = AdjustmentGetValue(table.hardness)
  local extend_value = AdjustmentGetValue(table.extend)
  local target = GetLayer(name)
  local alpha_value = AdjustmentGetValue(table.alpha)
  local under = GetBlendedUnderLayer(target, false)
  local use_under = ToggleButtonGetActive(table.use_under)
  local adjust_bright = AdjustmentGetValue(table.bright)

  alpha_value = alpha_value * 0.01 * 255

  if target == nil then
    ScriptEnd()
    return
  end

  if target.num_lines > 0 then
    for i = 1, target.num_lines do
      for j = 1, target.lines[i].num_points do
        local x = math.floor(target.lines[i].points[j].x)
        local y = math.floor(target.lines[i].points[j].y)
        if (under.pixels[y] ~= nil and under.pixels[y][x] ~= nil) == true then
          local ur, ug, ub, ua
          local r, g, b, a = ParseRGBA(target.lines[i].points[j].color)
          if use_under == true then
            ur, ug, ub, ua = ParseRGBA(GetAverageColor(under, x, y, 20))
            local h, s, v = RGB2HSV(ur, ug, ub)
            v = v + adjust_bright
            r, g, b = HSV2RGB(h, s, v)
          end
          target.lines[i].points[j].color = MergeRGBA(r, g, b, alpha_value)
          target.lines[i].blur = blur_value
          target.lines[i].outline_hardness = hardness_value
          target.lines[i].points[j].size = target.lines[i].points[j].size + extend_value
        end
      end
    end
  end

  UpdateVectorLayer(target)

  ScriptEnd()
end

function main()
  local has_vector_layer = false
  local window = WindowNew("ベクトル色調整")
  local box = VBoxNew(false, 0)
  local combo = ComboBoxNew()
  local button = ButtonNew("調整")
  local use_under_color = CheckButtonNew("下のレイヤーの合成色を使う")
  local blur_adjust = AdjustmentNew(0, 0, 100, 1, 1, 0)
  local hardness_adjust = AdjustmentNew(0, 0, 100, 1, 1, 0)
  local alpha_adjust = AdjustmentNew(100, 0, 100, 1, 1, 0)
  local extend_adjust = AdjustmentNew(0, -10, 10, 1, 1, 0)
  local brightness_adjust = AdjustmentNew(0, -100, 100, 1, 1, 0)
  local bottom = GetBottomLayerInfo()
  local layer = bottom
  local table = {
    combo_box = combo,
    blur = blur_adjust,
    hardness = hardness_adjust,
    alpha = alpha_adjust,
    extend = extend_adjust,
    use_under = use_under_color,
    bright = brightness_adjust
  }
  while layer ~= nil do
    if layer.type == TYPE_VECTOR_LAYER then
      has_vector_layer = true
      ComboBoxPrependText(combo, layer.name)
    end
    layer = GetNextLayerInfo(layer)
  end

  local hbox = HBoxNew(false, 0)
  local label = LabelNew("対象レイヤー")
  BoxPackStart(hbox, label, false, false, 0)
  BoxPackStart(hbox, combo, false, false, 0)
  BoxPackStart(box, hbox, false, false, 0)
  ComboBoxSetActive(combo, 0)

  label = LabelNew("ボケ足")
  local scale = HScaleNew(blur_adjust)
  hbox = HBoxNew(false, 0)
  BoxPackStart(hbox, label, false, false, 0)
  BoxPackStart(hbox, scale, true, true, 0)
  BoxPackStart(box, hbox, true, true, 0)

  label = LabelNew("輪郭の硬さ")
  scale = HScaleNew(hardness_adjust)
  hbox = HBoxNew(false, 0)
  BoxPackStart(hbox, label, false, false, 0)
  BoxPackStart(hbox, scale, true, true, 0)
  BoxPackStart(box, hbox, true, true, 0)

  label = LabelNew("濃度")
  scale = HScaleNew(alpha_adjust)
  hbox = HBoxNew(false, 0)
  BoxPackStart(hbox, label, false, false, 0)
  BoxPackStart(hbox, scale, true, true, 0)
  BoxPackStart(box, hbox, true, true, 0)

  label = LabelNew("拡大・縮小")
  scale = HScaleNew(extend_adjust)
  hbox = HBoxNew(false, 0)
  BoxPackStart(hbox, label, false, false, 0)
  BoxPackStart(hbox, scale, true, true, 0)
  BoxPackStart(box, hbox, true, true, 0)

  BoxPackStart(box, use_under_color, false, false, 0)

  scale = SpinScaleNew(brightness_adjust, "明るさ調整", 1)
  BoxPackStart(box, scale, true, true, 0)

  BoxPackStart(box, button, false, false, 0)
  SignalConnect(button, "clicked", button_clicked, table)

  ContainerAdd(window, box)

  MainLoop()

end
