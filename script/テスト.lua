function select_layer_changed(widget, layer)
  print(ComboBoxGetValue(widget))
end

function clicked(widget, layer)
  math.randomseed(os.time())
  for i = 1, layer.height do
    for j = 1, layer.width do
      layer.pixels[i][j] = MergeRGBA(
        math.random(1, 256)-1, math.random(1,256)-1, math.random(1,256)-1, 255)
    end
  end
  UpdatePixels(layer)
end

function main()
  local window = WindowNew("テストスクリプト")
  local box = VBoxNew(false, 0)
  local button = ButtonNew("Update")
  local combo = ComboBoxNew()
  local layer = GetBottomLayer()
  local bottom = layer
  while layer ~= nil do
    ComboBoxAppendText(combo, layer.name)
    layer = GetNextLayer(layer)
  end
  SignalConnect(combo, "changed", select_layer_changed, bottom)
  SignalConnect(button, "clicked", clicked, bottom)
  ContainerAdd(window, box)
  BoxPackStart(box, combo, false, false, 0)
  BoxPackStart(box, button, false, false, 0)
  MainLoop()
end
