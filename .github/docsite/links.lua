local docpath = ""
local repo = "https://github.com/gen2brain/iup-go/blob/main/"

local function dirname(p)
  return p:match("^(.*)/[^/]*$") or ""
end

local function normalize(p)
  local out = {}
  for part in p:gmatch("[^/]+") do
    if part == ".." then
      if #out > 0 and out[#out] ~= ".." then table.remove(out) else table.insert(out, "..") end
    elseif part ~= "." then
      table.insert(out, part)
    end
  end
  return table.concat(out, "/")
end

local function rewrite(target)
  if target:match("^%a[%w+.-]*:") or target:match("^#") or target == "" then
    return target
  end
  local path, frag = target:match("^([^#]*)(#?.*)$")
  local base = dirname(docpath)
  local full = normalize((base ~= "" and base .. "/" or "") .. path)
  if full:match("^%.%.") then
    local repo_path = normalize("docs/" .. full)
    return repo .. repo_path .. frag
  end
  if path:match("README%.md$") then
    path = path:gsub("README%.md$", "index.html")
  else
    path = path:gsub("%.md$", ".html")
  end
  return path .. frag
end

function Meta(meta)
  if meta.docpath then docpath = pandoc.utils.stringify(meta.docpath) end
end

function Link(el)
  el.target = rewrite(el.target)
  return el
end

function Image(el)
  el.src = rewrite(el.src)
  return el
end

function Pandoc(doc)
  if not doc.meta.pagetitle then
    for _, b in ipairs(doc.blocks) do
      if b.t == "Header" then
        doc.meta.pagetitle = pandoc.utils.stringify(b.content)
        break
      end
    end
  end
  return doc
end

return {{Meta = Meta}, {Link = Link, Image = Image, Pandoc = Pandoc}}
