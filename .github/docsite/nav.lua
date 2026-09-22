local prefix = ""
local nested = { ["Attributes"] = true, ["Events and Callbacks"] = true, ["Dialogs"] = true, ["Controls"] = true, ["Resources"] = true }

local function esc(s)
  return (s:gsub("&", "&amp;"):gsub("<", "&lt;"):gsub(">", "&gt;"):gsub('"', "&quot;"))
end

local function new_group(title)
  return { title = title, links = {}, subs = {} }
end

function Pandoc(doc)
  if doc.meta.navprefix then prefix = pandoc.utils.stringify(doc.meta.navprefix) end
  local groups, current, target = {}, nil, nil
  for _, b in ipairs(doc.blocks) do
    if b.t == "Header" and b.level == 2 then
      current = new_group(pandoc.utils.stringify(b.content))
      target = current
      table.insert(groups, current)
    elseif b.t == "Header" and b.level == 3 and current and nested[current.title] then
      target = new_group(pandoc.utils.stringify(b.content))
      table.insert(current.subs, target)
    elseif target then
      local function add(l)
        local t = l.target
        if not t:match("^%a[%w+.-]*:") then
          t = t:gsub("README%.md", "index.html"):gsub("%.md", ".html")
          local text = pandoc.utils.stringify(l.content)
          if text ~= "" then
            table.insert(target.links, { text = text, href = prefix .. t })
          end
        end
      end
      if b.t == "Table" then
        for _, body in ipairs(b.bodies) do
          for _, row in ipairs(body.body) do
            local first = nil
            pandoc.Div(row.cells[1].contents):walk({ Link = function(l) if not first then first = l end end })
            if first then add(first) end
          end
        end
      else
        b:walk({ Link = add })
      end
    end
  end

  local html = { '<nav class="sidebar-nav">', '<a class="nav-home" href="' .. prefix .. 'index.html">Overview</a>' }
  local function emit_links(links)
    local seen = {}
    table.insert(html, '<ul>')
    for _, l in ipairs(links) do
      if not seen[l.href] then
        seen[l.href] = true
        table.insert(html, '<li><a href="' .. esc(l.href) .. '">' .. esc(l.text) .. '</a></li>')
      end
    end
    table.insert(html, '</ul>')
  end
  for _, g in ipairs(groups) do
    table.insert(html, '<details><summary>' .. esc(g.title) .. '</summary>')
    if #g.links > 0 then emit_links(g.links) end
    if #g.subs > 0 then
      table.insert(html, '<div class="nav-subgroups">')
      for _, s in ipairs(g.subs) do
        table.insert(html, '<details><summary>' .. esc(s.title) .. '</summary>')
        emit_links(s.links)
        table.insert(html, '</details>')
      end
      table.insert(html, '</div>')
    end
    table.insert(html, '</details>')
  end
  table.insert(html, '</nav>')
  return pandoc.Pandoc({ pandoc.RawBlock("html", table.concat(html, "\n")) })
end
