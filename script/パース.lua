function parse_mode_toggled(button, data)
  if ToggleButtonGetActive(button) == true then
    data.parse_mode = ObjectGetData(button, "parse_mode")
  end
end

function divide3_toggled(widget, data)
  QueueDraw(data.draw_area)
end

function get_line_param(arg, point)
  local a = math.tan(arg)
  local b = point.y - a * point.x
  local rev_a
  local rev_b

  if a ~= 0 then
     rev_a = 1 / a
     rev_b = point.x - rev_a * point.y
  end

  return a, b, rev_a, rev_b
end

function draw_parse_line(cairo, data, point, disp_info)
  local cx = disp_info.width / 2
  local cy = disp_info.height / 2
  local arg = 0
  local x, y
  local a, rev_a, b, rev_b
  local div
  local current_point
  local draw_point = {}
  
  draw_point[1] = {x=disp_info.x, y=disp_info.y}
  draw_point[2] = {x=disp_info.x, y=disp_info.y+disp_info.disp_height}
  draw_point[3] = {x=disp_info.x+disp_info.disp_width, y=disp_info.y+disp_info.disp_height}
  draw_point[4] = {x=disp_info.x+disp_info.disp_width, y=disp_info.y}

  a = math.tan(arg)
  if point.x >= disp_info.x and point.x <= disp_info.x + disp_info.disp_width
      and point.y >= disp_info.y and point.y <= disp_info.y + disp_info.disp_height then
    div = (2 * math.pi) / data.num_line
    for i = 1, data.num_line do
      a, b, rev_a, rev_b = get_line_param(arg, point)
      if arg < math.pi / 2 then
        if a ~= nil then
          y = a * (disp_info.x + disp_info.disp_width) + b
          if y >= disp_info.y and y <= disp_info.y + disp_info.disp_height then
            CairoMoveTo(cairo, point.x, point.y)
            CairoLineTo(cairo, disp_info.x + disp_info.disp_width, y)
            CairoStroke(cairo)
          else
            if rev_a ~= nil then
              x = rev_a * disp_info.y + rev_b
              CairoMoveTo(cairo, point.x, point.y)
              CairoLineTo(cairo, x, disp_info.y)
              CairoStroke(cairo)
            end
          end
        else
          CairoMoveTo(cairo, point.x, point.y)
          CairoLineTo(cairo, point.x, disp_info.y)
          CairoStroke(cairo)
        end
      elseif arg < math.pi then
        if a ~= nil then
          y = a * disp_info.x + b
          if y >= disp_info.y and y <= disp_info.y + disp_info.disp_height then
            CairoMoveTo(cairo, point.x, point.y)
            CairoLineTo(cairo, disp_info.x, y)
            CairoStroke(cairo)
          else
            if rev_a ~= nil then
              x = rev_a * disp_info.y + rev_b
              CairoMoveTo(cairo, point.x, point.y)
              CairoLineTo(cairo, x, disp_info.y)
              CairoStroke(cairo)
            end
          end
        else
          CairoMoveTo(cairo, point.x, point.y)
          CairoLineTo(cairo, point.x, disp_info.y)
          CairoStroke(cairo)
        end
      elseif arg < math.pi + math.pi / 2 then
        if a ~= nil then
          y = a * disp_info.x + b
          if y >= disp_info.y and y <= disp_info.y + disp_info.disp_height then
            CairoMoveTo(cairo, point.x, point.y)
            CairoLineTo(cairo, disp_info.x, y)
            CairoStroke(cairo)
          else
            if rev_a ~= nil then
              x = rev_a * (disp_info.y + disp_info.disp_height) + rev_b
              CairoMoveTo(cairo, point.x, point.y)
              CairoLineTo(cairo, x, disp_info.y + disp_info.disp_height)
              CairoStroke(cairo)
            end
          end
        else
          CairoMoveTo(cairo, point.x, point.y)
          CairoLineTo(cairo, point.x, disp_info.y)
          CairoStroke(cairo)
        end
      else
        if a ~= nil then
          y = a * (disp_info.x + disp_info.disp_width) + b
          if y >= disp_info.y and y <= disp_info.y + disp_info.disp_height then
            CairoMoveTo(cairo, point.x, point.y)
            CairoLineTo(cairo, disp_info.x + disp_info.disp_width, y)
            CairoStroke(cairo)
          else
            if rev_a ~= nil then
              x = rev_a * (disp_info.y + disp_info.disp_height) + rev_b
              CairoMoveTo(cairo, point.x, point.y)
              CairoLineTo(cairo, x, disp_info.y + disp_info.disp_height)
              CairoStroke(cairo)
            end
          end
        else
          CairoMoveTo(cairo, point.x, point.y)
          CairoLineTo(cairo, point.x, disp_info.y)
          CairoStroke(cairo)
        end
      end

      arg = arg + div
    end
  else
    local max_arg, min_arg
    local index1=1
    local index2=2
    max_arg = 0
    for i=1, 4 do
      for j=i+1, 4 do
        arg = math.abs(math.atan2(point.y-draw_point[i].y, point.x-draw_point[i].x)
          - math.atan2(point.y-draw_point[j].y, point.x-draw_point[j].x))
        if arg >= math.pi then
          arg = math.pi * 2 - arg
        end

        if max_arg < arg then
          max_arg = arg
          index1 = i
          index2 = j
        end
      end
    end

    div = max_arg / data.num_line
    max_arg = math.atan2(point.y-draw_point[index1].y, point.x-draw_point[index1].x)
    while max_arg < 0 do
      max_arg = max_arg + math.pi
    end
    min_arg = math.atan2(point.y-draw_point[index2].y, point.x-draw_point[index2].x)
    while min_arg < 0 do
      min_arg = min_arg + math.pi
    end

    if max_arg < min_arg then
      max_arg, min_arg = min_arg, max_arg
    end
    if max_arg - min_arg > math.pi then
      div = - div
    end

    if (index1 == 1 and index2 == 2)
        or (index1 == 3 and index2 == 4) then
      min_arg = - min_arg
    end

    arg = min_arg + div
    for i = 1, data.num_line do
      current_point = 1
      a, b, rev_a, rev_b = get_line_param(arg, point)
      if a ~= nil then
        y = a * disp_info.x + b

        if y >= disp_info.y and y <= disp_info.y + disp_info.disp_height then
          draw_point[current_point].x = disp_info.x
          draw_point[current_point].y = y
          current_point = current_point + 1
        end

        y = a * (disp_info.x + disp_info.disp_width) + b
        if y >= disp_info.y and y <= disp_info.y + disp_info.disp_height then
          draw_point[current_point].x = disp_info.x + disp_info.disp_width
          draw_point[current_point].y = y
          current_point = current_point + 1
        end

        if rev_a ~= nil then
          x = rev_a * disp_info.y + rev_b
          if x >= disp_info.x and x <= disp_info.x + disp_info.disp_width then
            draw_point[current_point].x = x
            draw_point[current_point].y = disp_info.y
            current_point = current_point + 1
          end

          x = rev_a * (disp_info.y + disp_info.disp_height) + rev_b
          if x >= disp_info.x and x <= disp_info.x + disp_info.disp_width then
            draw_point[current_point].x = x
            draw_point[current_point].y = disp_info.y + disp_info.disp_height
            current_point = current_point + 1
          end
        end
      end

      CairoMoveTo(cairo, draw_point[1].x, draw_point[1].y)
      CairoLineTo(cairo, draw_point[2].x, draw_point[2].y)
      CairoStroke(cairo)

      arg = arg + div
    end
  end
end

function expose(widget, cairo, data)
  local width = 0
  local height = 0
  local size = 0
  local x, y
  local disp_width, disp_height
  local disp_info = {}

  CairoSave(cairo)

  width, height = GetWidgetSize(widget)

  if data.canvas.width > data.canvas.height then
    size = data.canvas.width
  else
    size = data.canvas.height
  end

  if width < size then
    zoom = (width / size) * 0.5
  else
    zoom = 0.5
  end

  disp_width = data.canvas.width * zoom
  disp_height = data.canvas.height * zoom

  x = (width - data.canvas.width*zoom) * 0.5
  y = (height - data.canvas.height*zoom) * 0.5
  CairoTranslate(cairo, x, y)

  CairoScale(cairo, zoom, zoom)
  CairoSetOperator(cairo, CAIRO_OPERATOR_OVER)
  CairoSetSourceSurface(cairo, data.canvas.surface)
  CairoPaint(cairo)

  CairoRestore(cairo)

  if ToggleButtonGetActive(data.divide3) == true then
    CairoSetSourceRGB(cairo, 0, 0, 0)
    CairoMoveTo(cairo, x + disp_width / 3, y)
    CairoLineTo(cairo, x + disp_width / 3, y + disp_height)
    CairoStroke(cairo)
    CairoMoveTo(cairo, x + (disp_width / 3) * 2, y)
    CairoLineTo(cairo, x + (disp_width / 3) * 2, y + disp_height)
    CairoStroke(cairo)
    CairoMoveTo(cairo, x, y + disp_height / 3)
    CairoLineTo(cairo, x + disp_width, y + disp_height / 3)
    CairoStroke(cairo)
    CairoMoveTo(cairo, x, y + (disp_height / 3) * 2)
    CairoLineTo(cairo, x + disp_width, y + (disp_height / 3) * 2)
    CairoStroke(cairo)
  end

  disp_info = {x = x, y = y, width = width, height = height,
    zoom = zoom, disp_width = disp_width, disp_height = disp_height}

  if data.num_click >= 1 then
    CairoSetSourceRGB(cairo, data.color1.r/255.0, data.color1.g/255.0, data.color1.b/255.0)
    draw_parse_line(cairo, data, data.point_table[1], disp_info)
  end
  if data.num_click >= 2 and data.parse_mode >= 2 then
    CairoSetSourceRGB(cairo, data.color2.r/255.0, data.color2.g/255.0, data.color2.b/255.0)
    draw_parse_line(cairo, data, data.point_table[2], disp_info)
  end
  if data.num_click >= 3 and data.parse_mode >= 3 then
    CairoSetSourceRGB(cairo, data.color3.r/255.0, data.color3.g/255.0, data.color3.b/255.0)
    draw_parse_line(cairo, data, data.point_table[3], disp_info)
  end
end

function button_press(widget, mouse, data)
  if bit32.band(mouse.state, BUTTON1_MASK) ~= 0 then
    data.point_table[data.current_point].x = mouse.x
    data.point_table[data.current_point].y = mouse.y
    data.num_click = data.num_click + 1
    data.current_point = data.current_point + 1
    if data.num_click > data.parse_mode then
      data.num_click = data.parse_mode
    end
    if data.current_point > data.parse_mode then
      data.current_point = 1
    end

    QueueDraw(data.draw_area)
  end
end

function motion(widget, mouse, dummy)
  
  -- QueueDraw(widget)
end

function add_parse_line(layer, data, point, disp_info, color)
  local cx = disp_info.width / 2
  local cy = disp_info.height / 2
  local arg = 0
  local x, y
  local a, rev_a, b, rev_b
  local div
  local current_point
  local rev_zoom = 1 / disp_info.zoom
  local flags = 0
  local draw_point = {}
  local vector_point = {}
  vector_point[1] = {x=0, y=0, vector_type = VECTOR_LINE_STRAIGHT,
    color=MergeRGBA(color.r, color.g, color.b, 255), pressure=100, size=4, flags=0}
  vector_point[2] = {x=0, y=0, vector_type = VECTOR_LINE_STRAIGHT,
    color=MergeRGBA(color.r, color.g, color.b, 255), pressure=100, size=4, flags=0}

  if ToggleButtonGetActive(data.anti_alias) == true then
    flags = bit32.bor(flags, VECTOR_LINE_ANTI_ALIAS)
  end
  
  draw_point[1] = {x=disp_info.x, y=disp_info.y}
  draw_point[2] = {x=disp_info.x, y=disp_info.y+disp_info.disp_height}
  draw_point[3] = {x=disp_info.x+disp_info.disp_width, y=disp_info.y+disp_info.disp_height}
  draw_point[4] = {x=disp_info.x+disp_info.disp_width, y=disp_info.y}

  a = math.tan(arg)
  if point.x >= disp_info.x and point.x <= disp_info.x + disp_info.disp_width
      and point.y >= disp_info.y and point.y <= disp_info.y + disp_info.disp_height then
    div = (2 * math.pi) / data.num_line
    for i = 1, data.num_line do
      a, b, rev_a, rev_b = get_line_param(arg, point)
      if arg < math.pi / 2 then
        if a ~= nil then
          y = a * (disp_info.x + disp_info.disp_width) + b
          if y >= disp_info.y and y <= disp_info.y + disp_info.disp_height then
            vector_point[1].x = (point.x - disp_info.x) * rev_zoom
            vector_point[1].y = (point.y - disp_info.y) * rev_zoom
            vector_point[2].x = layer.width
            vector_point[2].y = (y - disp_info.y) * rev_zoom
            AddVectorLine(layer, VECTOR_LINE_STRAIGHT, 1, 0, flags, vector_point[1], vector_point[2])
          else
            if rev_a ~= nil then
              x = rev_a * disp_info.y + rev_b
              vector_point[1].x = (point.x - disp_info.x) * rev_zoom
              vector_point[1].y = (point.y - disp_info.y) * rev_zoom
              vector_point[2].x = (x - disp_info.x) * rev_zoom
              vector_point[2].y = 0
              AddVectorLine(layer, VECTOR_LINE_STRAIGHT, 1, 0, flags, vector_point[1], vector_point[2])
            end
          end
        else
          vector_point[1].x = (point.x - disp_info.x) * rev_zoom
          vector_point[1].y = 0
          vector_point[2].x = (point.x - disp_info.x) * rev_zoom
          vector_point[2].y = layer.height
          AddVectorLine(layer, VECTOR_LINE_STRAIGHT, 1, 0, flags, vector_point[1], vector_point[2])
        end
      elseif arg < math.pi then
        if a ~= nil then
          y = a * disp_info.x + b
          if y >= disp_info.y and y <= disp_info.y + disp_info.disp_height then
            vector_point[1].x = (point.x - disp_info.x) * rev_zoom
            vector_point[1].y = (point.y - disp_info.y) * rev_zoom
            vector_point[2].x = 0
            vector_point[2].y = (y - disp_info.y) * rev_zoom
            AddVectorLine(layer, VECTOR_LINE_STRAIGHT, 1, 0, flags, vector_point[1], vector_point[2])
          else
            if rev_a ~= nil then
              x = rev_a * disp_info.y + rev_b
              vector_point[1].x = (point.x - disp_info.x) * rev_zoom
              vector_point[1].y = (point.y - disp_info.y) * rev_zoom
              vector_point[2].x = (x - disp_info.x) * rev_zoom
              vector_point[2].y = 0
              AddVectorLine(layer, VECTOR_LINE_STRAIGHT, 1, 0, flags, vector_point[1], vector_point[2])
            end
          end
        else
          vector_point[1].x = (point.x - disp_info.x) * rev_zoom
          vector_point[1].y = 0
          vector_point[2].x = (point.x - disp_info.x) * rev_zoom
          vector_point[2].y = layer.height
          AddVectorLine(layer, VECTOR_LINE_STRAIGHT, 1, 0, flags, vector_point[1], vector_point[2])
        end
      elseif arg < math.pi + math.pi / 2 then
        if a ~= nil then
          y = a * disp_info.x + b
          if y >= disp_info.y and y <= disp_info.y + disp_info.disp_height then
            vector_point[1].x = (point.x - disp_info.x) * rev_zoom
            vector_point[1].y = (point.y - disp_info.y) * rev_zoom
            vector_point[2].x = 0
            vector_point[2].y = (y - disp_info.y) * rev_zoom
            AddVectorLine(layer, VECTOR_LINE_STRAIGHT, 1, 0, flags, vector_point[1], vector_point[2])
          else
            if rev_a ~= nil then
              x = rev_a * (disp_info.y + disp_info.disp_height) + rev_b
              vector_point[1].x = (point.x - disp_info.x) * rev_zoom
              vector_point[1].y = (point.y - disp_info.y) * rev_zoom
              vector_point[2].x = (x - disp_info.x) * rev_zoom
              vector_point[2].y = layer.height
              AddVectorLine(layer, VECTOR_LINE_STRAIGHT, 1, 0, flags, vector_point[1], vector_point[2])
            end
          end
        else
          vector_point[1].x = (point.x - disp_info.x) * rev_zoom
          vector_point[1].y = 0
          vector_point[2].x = (point.x - disp_info.x) * rev_zoom
          vector_point[2].y = layer.height
          AddVectorLine(layer, VECTOR_LINE_STRAIGHT, 1, 0, flags, vector_point[1], vector_point[2])
        end
      else
        if a ~= nil then
          y = a * (disp_info.x + disp_info.disp_width) + b
          if y >= disp_info.y and y <= disp_info.y + disp_info.disp_height then
            vector_point[1].x = (point.x - disp_info.x) * rev_zoom
            vector_point[1].y = (point.y - disp_info.y) * rev_zoom
            vector_point[2].x = layer.width
            vector_point[2].y = (y - disp_info.y) * rev_zoom
            AddVectorLine(layer, VECTOR_LINE_STRAIGHT, 1, 0, flags, vector_point[1], vector_point[2])
          else
            if rev_a ~= nil then
              x = rev_a * (disp_info.y + disp_info.disp_height) + rev_b
              vector_point[1].x = (point.x - disp_info.x) * rev_zoom
              vector_point[1].y = (point.y - disp_info.y) * rev_zoom
              vector_point[2].x = (x - disp_info.x) * rev_zoom
              vector_point[2].y = layer.height
              AddVectorLine(layer, VECTOR_LINE_STRAIGHT, 1, 0, flags, vector_point[1], vector_point[2])
            end
          end
        else
          vector_point[1].x = (point.x - disp_info.x) * rev_zoom
          vector_point[1].y = 0
          vector_point[2].x = (point.x - disp_info.x) * rev_zoom
          vector_point[2].y = layer.height
          AddVectorLine(layer, VECTOR_LINE_STRAIGHT, 1, 0, flags, vector_point[1], vector_point[2])
        end
      end

      arg = arg + div
    end
  else
    local max_arg, min_arg
    local index1=1
    local index2=2
    max_arg = 0
    for i=1, 4 do
      for j=i+1, 4 do
        arg = math.abs(math.atan2(point.y-draw_point[i].y, point.x-draw_point[i].x)
          - math.atan2(point.y-draw_point[j].y, point.x-draw_point[j].x))
        if arg >= math.pi then
          arg = math.pi * 2 - arg
        end

        if max_arg < arg then
          max_arg = arg
          index1 = i
          index2 = j
        end
      end
    end

    div = max_arg / data.num_line
    max_arg = math.atan2(point.y-draw_point[index1].y, point.x-draw_point[index1].x)
    while max_arg < 0 do
      max_arg = max_arg + math.pi
    end
    min_arg = math.atan2(point.y-draw_point[index2].y, point.x-draw_point[index2].x)
    while min_arg < 0 do
      min_arg = min_arg + math.pi
    end

    if max_arg < min_arg then
      max_arg, min_arg = min_arg, max_arg
    end
    if max_arg - min_arg > math.pi then
      div = - div
    end

    if (index1 == 1 and index2 == 2)
        or (index1 == 3 and index2 == 4) then
      min_arg = - min_arg
    end

    arg = min_arg + div
    for i = 1, data.num_line do
      current_point = 1
      a, b, rev_a, rev_b = get_line_param(arg, point)
      if a ~= nil then
        y = a * disp_info.x + b

        if y >= disp_info.y and y <= disp_info.y + disp_info.disp_height then
          draw_point[current_point].x = disp_info.x
          draw_point[current_point].y = y
          current_point = current_point + 1
        end

        y = a * (disp_info.x + disp_info.disp_width) + b
        if y >= disp_info.y and y <= disp_info.y + disp_info.disp_height then
          draw_point[current_point].x = disp_info.x + disp_info.disp_width
          draw_point[current_point].y = y
          current_point = current_point + 1
        end

        if rev_a ~= nil then
          x = rev_a * disp_info.y + rev_b
          if x >= disp_info.x and x <= disp_info.x + disp_info.disp_width then
            draw_point[current_point].x = x
            draw_point[current_point].y = disp_info.y
            current_point = current_point + 1
          end

          x = rev_a * (disp_info.y + disp_info.disp_height) + rev_b
          if x >= disp_info.x and x <= disp_info.x + disp_info.disp_width then
            draw_point[current_point].x = x
            draw_point[current_point].y = disp_info.y + disp_info.disp_height
            current_point = current_point + 1
          end
        end
      end

      vector_point[1].x = (draw_point[1].x - disp_info.x) * rev_zoom
      vector_point[1].y = (draw_point[1].y - disp_info.y) * rev_zoom
      vector_point[2].x = (draw_point[2].x - disp_info.x) * rev_zoom
      vector_point[2].y = (draw_point[2].y - disp_info.y) * rev_zoom
      AddVectorLine(layer, VECTOR_LINE_STRAIGHT, 1, 0, flags, vector_point[1], vector_point[2])

      arg = arg + div
    end
  end
end

function ok_button_clicked(widget, data)
  local layer = GetBottomLayerInfo()
  local top_name = layer.name
  local new_layer
  local point = {}
  point[1] = {x=0, y=0, vector_type = VECTOR_LINE_STRAIGHT,
    color=MergeRGBA(0, 0, 0, 255), pressure=100, size=6, flags=0}
  point[2] = {x=0, y=0, vector_type = VECTOR_LINE_STRAIGHT,
    color=MergeRGBA(0, 0, 0, 255), pressure=100, size=6, flags=0}

  while layer ~= nil do
    top_name = layer.name
    layer = GetNextLayerInfo(layer)
  end

  new_layer = NewLayer("パース", top_name, TYPE_VECTOR_LAYER)

  local width = 0
  local height = 0
  local size = 0
  local x, y
  local disp_width, disp_height
  local disp_info = {}

  width, height = GetWidgetSize(data.draw_area)

  if data.canvas.width > data.canvas.height then
    size = data.canvas.width
  else
    size = data.canvas.height
  end

  if width < size then
    zoom = (width / size) * 0.5
  else
    zoom = 0.5
  end

  disp_width = data.canvas.width * zoom
  disp_height = data.canvas.height * zoom

  x = (width - data.canvas.width*zoom) * 0.5
  y = (height - data.canvas.height*zoom) * 0.5

  if ToggleButtonGetActive(data.divide3) == true then
    point[1].x = data.canvas.width / 3
    point[1].y = 0
    point[2].x = point[1].x
    point[2].y = data.canvas.height
    AddVectorLine(new_layer, VECTOR_LINE_STRAIGHT, 1, 0, 0, point[1], point[2])
    point[1].x = point[1].x * 2
    point[2].x = point[1].x
    AddVectorLine(new_layer, VECTOR_LINE_STRAIGHT, 1, 0, 0, point[1], point[2])
    point[1].x = 0
    point[1].y = data.canvas.height / 3
    point[2].x = data.canvas.width
    point[2].y = point[1].y
    AddVectorLine(new_layer, VECTOR_LINE_STRAIGHT, 1, 0, 0, point[1], point[2])
    point[1].y = point[1].y * 2
    point[2].y = point[1].y
    AddVectorLine(new_layer, VECTOR_LINE_STRAIGHT, 1, 0, 0, point[1], point[2])
  end

  disp_info = {x = x, y = y, width = width, height = height,
    zoom = zoom, disp_width = disp_width, disp_height = disp_height}

  if data.num_click >= 1 then
    add_parse_line(new_layer, data, data.point_table[1], disp_info, data.color1)
  end
  if data.num_click >= 2 and data.parse_mode >= 2 then
    add_parse_line(new_layer, data, data.point_table[2], disp_info, data.color2)
  end
  if data.num_click >= 3 and data.parse_mode >= 3 then
    add_parse_line(new_layer, data, data.point_table[3], disp_info, data.color3)
  end

  UpdateVectorLayer(new_layer)

  ScriptEnd()
end

function color1_button_clicked(button, data)
  local dialog = ColorSelectionDialogNew("線の色を選択")
  local selection = ColorSelectionDialogGetColorSelection(dialog)

  ColorSelectionSetCurrentColor(selection, data.color1.r, data.color1.g, data.color1.b)
  if DialogRun(dialog) == RESPONSE_OK then
    data.color1.r, data.color1.g, data.color1.b = ColorSelectionGetCurrentColor(selection)
  end

  WidgetDestroy(dialog)

  QueueDraw(data.color1.widget)
end

function color1_draw(widget, cairo, data)
  CairoSetSourceRGB(cairo, data.color1.r / 255.0, data.color1.g / 255.0, data.color1.b / 255.0)
  CairoMoveTo(cairo, 0, 0)
  CairoLineTo(cairo, 0, 16)
  CairoLineTo(cairo, 16, 16)
  CairoLineTo(cairo, 16, 0)
  CairoFill(cairo)
end

function color2_button_clicked(button, data)
  local dialog = ColorSelectionDialogNew("線の色を選択")
  local selection = ColorSelectionDialogGetColorSelection(dialog)

  ColorSelectionSetCurrentColor(selection, data.color2.r, data.color2.g, data.color2.b)
  if DialogRun(dialog) == RESPONSE_OK then
    data.color2.r, data.color2.g, data.color2.b = ColorSelectionGetCurrentColor(selection)
  end

  WidgetDestroy(dialog)

  QueueDraw(data.color2.widget)
end

function color2_draw(widget, cairo, data)
  CairoSetSourceRGB(cairo, data.color2.r / 255.0, data.color2.g / 255.0, data.color2.b / 255.0)
  CairoMoveTo(cairo, 0, 0)
  CairoLineTo(cairo, 0, 16)
  CairoLineTo(cairo, 16, 16)
  CairoLineTo(cairo, 16, 0)
  CairoFill(cairo)
end

function color3_button_clicked(button, data)
  local dialog = ColorSelectionDialogNew("線の色を選択")
  local selection = ColorSelectionDialogGetColorSelection(dialog)

  ColorSelectionSetCurrentColor(selection, data.color3.r, data.color3.g, data.color3.b)
  if DialogRun(dialog) == RESPONSE_OK then
    data.color3.r, data.color3.g, data.color3.b = ColorSelectionGetCurrentColor(selection)
  end

  WidgetDestroy(dialog)

  QueueDraw(data.color3.widget)
end

function color3_draw(widget, cairo, data)
  CairoSetSourceRGB(cairo, data.color3.r / 255.0, data.color3.g / 255.0, data.color3.b / 255.0)
  CairoMoveTo(cairo, 0, 0)
  CairoLineTo(cairo, 0, 16)
  CairoLineTo(cairo, 16, 16)
  CairoLineTo(cairo, 16, 0)
  CairoFill(cairo)
end

function main()
  local window = WindowNew("パーススクリプト")
  local ScriptData = {
    canvas = GetCanvasLayer(),
    draw_area = DrawingAreaNew(),
    parse_mode = 1,
    num_line = 15,
    point_table = {},
    current_point = 1,
    num_click = 0,
    point1 = RadioButtonNew(nil, "1点透視"),
    point2,
    point3,
    divide3 = CheckButtonNew("3分割"),
    anti_alias = CheckButtonNew("アンチエイリアス"),
    color1 = {r = 0, g = 0, b = 0},
    color2 = {r = 0, g = 0, b = 0},
    color3 = {r = 0, g = 0, b = 0}
  }

  ScriptData.point2 = RadioButtonNew(
    RadioButtonGetGroup(ScriptData.point1), "2点透視")
  ScriptData.point3 = RadioButtonNew(
    RadioButtonGetGroup(ScriptData.point1), "3点透視")
  ObjectSetData(ScriptData.point1, "parse_mode", 1)
  ObjectSetData(ScriptData.point2, "parse_mode", 2)
  ObjectSetData(ScriptData.point3, "parse_mode", 3)

  ScriptData.point_table[1] = {x = 200, y = 200}
  ScriptData.point_table[2] = {x = 200, y = 200}
  ScriptData.point_table[3] = {x = 200, y = 200}

  local hbox = HBoxNew(false, 0)
  local ui = VBoxNew(false, 0)
  local color_button
  local ok_button = ButtonNew("OK")
  local num_line_adjust = AdjustmentNew(ScriptData.num_line, 3, 30, 1, 1, 0)
  local spin = SpinButtonNew(num_line_adjust)
  local label = LabelNew("線の本数")
  local spin_box = HBoxNew(false, 0)
  BoxPackStart(spin_box, label, false, false, 0)
  BoxPackStart(spin_box, spin, false, false, 0)

  ContainerAdd(window, hbox)
  BoxPackStart(hbox, ScriptData.draw_area, false, false, 0)
  BoxPackStart(hbox, SeparatorNew(true), false, false, 0)
  BoxPackStart(hbox, ui, false, false, 0)
  BoxPackStart(ui, ScriptData.point1, false, false, 0)
  BoxPackStart(ui, ScriptData.point2, false, false, 0)
  BoxPackStart(ui, ScriptData.point3, false, false, 0)
  BoxPackStart(ui, SeparatorNew(false), false, false, 0)
  BoxPackStart(ui, ScriptData.divide3, false, false, 0)
  BoxPackStart(ui, ScriptData.anti_alias, false, false, 0)
  BoxPackStart(ui, spin_box, false, false, 0)

  hbox = HBoxNew(false, 3)
  label = LabelNew("色")
  BoxPackStart(hbox, label, false, false, 0)

  color_button = ButtonNew()
  BoxPackStart(hbox, color_button, false, false, 0)
  ScriptData.color1.widget = DrawingAreaNew()
  SetEvents(ScriptData.color1.widget, EXPOSURE_MASK)
  SignalConnect(ScriptData.color1.widget, "expose_event", color1_draw, ScriptData)
  ContainerAdd(color_button, ScriptData.color1.widget)
  SignalConnect(color_button, "clicked", color1_button_clicked, ScriptData)
  SetSizeRequest(ScriptData.color1.widget, 16, 16)

  color_button = ButtonNew()
  BoxPackStart(hbox, color_button, false, false, 0)
  ScriptData.color2.widget = DrawingAreaNew()
  SetEvents(ScriptData.color2.widget, EXPOSURE_MASK)
  SignalConnect(ScriptData.color2.widget, "expose_event", color2_draw, ScriptData)
  ContainerAdd(color_button, ScriptData.color2.widget)
  SignalConnect(color_button, "clicked", color2_button_clicked, ScriptData)
  SetSizeRequest(ScriptData.color2.widget, 16, 16)

  color_button = ButtonNew()
  BoxPackStart(hbox, color_button, false, false, 0)
  ScriptData.color3.widget = DrawingAreaNew()
  SetEvents(ScriptData.color3.widget, EXPOSURE_MASK)
  SignalConnect(ScriptData.color3.widget, "expose_event", color3_draw, ScriptData)
  ContainerAdd(color_button, ScriptData.color3.widget)
  SignalConnect(color_button, "clicked", color3_button_clicked, ScriptData)
  SetSizeRequest(ScriptData.color3.widget, 16, 16)

  BoxPackStart(ui, hbox, false, false, 0)

  BoxPackEnd(ui, ok_button, false, false, 0)
  SetSizeRequest(ScriptData.draw_area, 400, 400)
  SetEvents(ScriptData.draw_area, bit32.bor(EXPOSURE_MASK,
    POINTER_MOTION_NOTIFY_MASK, BUTTON_PRESS_MASK))
  SignalConnect(ScriptData.draw_area, "expose_event", expose, ScriptData)
  SignalConnect(ScriptData.draw_area, "motion_notify_event", motion, ScriptData)
  SignalConnect(ScriptData.draw_area, "button_press_event", button_press, ScriptData)

  SignalConnect(ScriptData.point1, "toggled", parse_mode_toggled, ScriptData)
  SignalConnect(ScriptData.point2, "toggled", parse_mode_toggled, ScriptData)
  SignalConnect(ScriptData.point3, "toggled", parse_mode_toggled, ScriptData)

  SignalConnect(ScriptData.divide3, "toggled", divide3_toggled, ScriptData)

  SignalConnect(ok_button, "clicked", ok_button_clicked, ScriptData)

  MainLoop()
end
