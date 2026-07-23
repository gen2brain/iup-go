// DOM applier/reader/input-router. Runs as an emcc --pre-js on main (drives Module directly)
// or in worker mode (dispatch posts events to the Worker).
(function () {
  if (globalThis.__iupInstallApplier) return;
  globalThis.__iupInstallApplier = function (dispatch) {
    if (globalThis.__iupApply && globalThis.__iupApply.__iupReal) return;
    globalThis.__iup = globalThis.__iup || { els: {}, next: 1 };
    var els = globalThis.__iup.els;
    var D = function (name) { dispatch.call(name, Array.prototype.slice.call(arguments, 1)); };
    var Dr = function (name, ret, types, args) { return dispatch.callRet(name, ret, types, args); };
    var Dt = function (name, types, args) { dispatch.callType(name, types, args); };
    var mmods = function (e) { return (e.shiftKey ? 1 : 0) | (e.ctrlKey ? 2 : 0) | (e.altKey ? 4 : 0) | ((e.buttons & 1) ? 8 : 0) | ((e.buttons & 2) ? 16 : 0) | ((e.buttons & 4) ? 32 : 0); };

    // 1-based lin/col <-> char-offset over a multiline value
    function iupLcToPos(val, lin, col) {
      var lines = val.split('\n');
      if (lin < 1) lin = 1; if (lin > lines.length) lin = lines.length;
      var pos = 0;
      for (var i = 0; i < lin - 1; i++) pos += lines[i].length + 1;
      var c = (col < 1) ? 0 : col - 1;
      if (c > lines[lin - 1].length) c = lines[lin - 1].length;
      return pos + c;
    }
    function iupPosToLc(val, pos) {
      if (pos < 0) pos = 0; if (pos > val.length) pos = val.length;
      var before = val.slice(0, pos);
      return before.split('\n').length + ',' + (pos - before.lastIndexOf('\n'));
    }

    // Shared by node add and copy: rebuilds listeners per row since cloneNode drops them.
    function buildTreeRow(tree, treeId, rowId, isBranch, title) {
      var row = document.createElement('div');
      row.style.display = 'flex'; row.style.alignItems = 'center'; row.style.whiteSpace = 'nowrap'; row.style.cursor = 'default'; row.style.padding = '1px 4px';
      row.__iupRowId = rowId; row.__iupKind = isBranch ? 0 : 1; row.__iupExpanded = isBranch ? 1 : 0;
      if (tree.__iupDndSrc) row.draggable = true;
      var indent = document.createElement('span'); indent.style.flex = '0 0 auto';
      var exp = document.createElement('span');
      exp.style.width = '12px'; exp.style.display = 'inline-block'; exp.style.textAlign = 'center';
      exp.textContent = isBranch ? String.fromCharCode(0x25BE) : '';
      var icon = document.createElement('span');
      icon.textContent = (isBranch ? String.fromCodePoint(0x1F4C1) : String.fromCodePoint(0x1F4C4)) + ' ';
      var ttl = document.createElement('span'); ttl.textContent = title;
      row.appendChild(indent); row.appendChild(exp);
      if (tree.__iupShowToggle) {
        var chk = document.createElement('input'); chk.type = 'checkbox'; chk.style.margin = '0 3px 0 0';
        if (tree.__iupShowToggle === 2) chk.indeterminate = true;
        chk.addEventListener('mousedown', function (e) { e.stopPropagation(); });
        chk.addEventListener('change', function (e) { e.stopPropagation(); D('iupwasmTreeToggle', treeId, rowId, chk.indeterminate ? -1 : (chk.checked ? 1 : 0)); });
        row.appendChild(chk); row.__iupChk = chk;
      }
      row.appendChild(icon); row.appendChild(ttl);
      row.__iupTitleEl = ttl; row.__iupExpEl = exp; row.__iupIndent = indent; row.__iupIcon = icon;
      exp.addEventListener('mousedown', function (e) { e.stopPropagation(); D('iupwasmTreeEvent', treeId, rowId, 1); });
      row.addEventListener('mousedown', function (e) { if (e.button === 2) return; D('iupwasmTreeSelect', treeId, rowId, e.ctrlKey ? 1 : 0, e.shiftKey ? 1 : 0); });
      row.addEventListener('dblclick', function () { D('iupwasmTreeEvent', treeId, rowId, row.__iupKind === 1 ? 2 : 4); });
      row.addEventListener('contextmenu', function (e) { e.preventDefault(); D('iupwasmTreeEvent', treeId, rowId, 3); });
      return row;
    }

    // Renumbers dataset.col after the move: IUP logical col follows visual order, like GTK.
    function tableReorderCols(el, from, to) {
      var rows = [el.__iupHead].concat(Array.prototype.slice.call(el.__iupBody.children));
      rows.forEach(function (row) {
        var cells = Array.prototype.slice.call(row.children);
        var node = cells.splice(from - 1, 1)[0];
        if (!node) return;
        cells.splice(to - 1, 0, node);
        cells.forEach(function (cell, i) { cell.dataset.col = i + 1; row.appendChild(cell); });
      });
    }

    function tableRowLineOf(el, tr) {
      return Array.prototype.indexOf.call(el.__iupBody.children, tr) + 1;
    }

    function tableClearRowDrop(el) {
      if (el.__iupDropRow) { el.__iupDropRow.style.boxShadow = ''; el.__iupDropRow = null; }
    }

    function treeNodeIcon(row) {
      var tree = row.parentNode, icon = row.__iupIcon;
      if (!icon) return;
      var imgId = 0;
      if (row.__iupKind === 1) imgId = row.__iupImg || (tree && tree.__iupImgLeaf) || 0;
      else if (row.__iupExpanded) imgId = row.__iupImgExpanded || (tree && tree.__iupImgExpanded) || row.__iupImg || 0;
      else imgId = row.__iupImg || (tree && tree.__iupImgCollapsed) || 0;
      var im = imgId ? (globalThis.__iupImg && globalThis.__iupImg.map[imgId]) : null;
      if (im) {
        if (!row.__iupNodeImg) { row.__iupNodeImg = document.createElement('img'); row.__iupNodeImg.style.verticalAlign = 'middle'; row.__iupNodeImg.style.marginRight = '3px'; }
        row.__iupNodeImg.src = im.url; row.__iupNodeImg.width = im.w; row.__iupNodeImg.height = im.h;
        icon.textContent = ''; icon.appendChild(row.__iupNodeImg);
      } else {
        icon.textContent = (row.__iupKind === 0 ? String.fromCodePoint(0x1F4C1) : String.fromCodePoint(0x1F4C4)) + ' ';
      }
    }

    // Derive all colors from the IUP globals as CSS custom properties, so changing a global reskins everything.
    function installTheme(c) {
      var S = document.documentElement.style;
      function rgb(a) { return 'rgb(' + Math.round(a[0]) + ',' + Math.round(a[1]) + ',' + Math.round(a[2]) + ')'; }
      function mix(a, t, k) { return [a[0] + (t[0] - a[0]) * k, a[1] + (t[1] - a[1]) * k, a[2] + (t[2] - a[2]) * k]; }
      var W = [255, 255, 255], bg = c.bg, ac = c.accent;
      var dark = bg[0] * 0.3 + bg[1] * 0.59 + bg[2] * 0.11 < 128;
      var edge = dark ? W : [0, 0, 0];
      S.setProperty('color-scheme', dark ? 'dark' : 'light');   // themes native sub-parts: select arrow, spinners, caret
      S.setProperty('--iup-bg', rgb(bg));
      S.setProperty('--iup-fg', rgb(c.fg));
      S.setProperty('--iup-txtbg', rgb(c.txtbg));
      S.setProperty('--iup-txtfg', rgb(c.txtfg));
      S.setProperty('--iup-link', rgb(c.link));
      S.setProperty('--iup-menubg', rgb(c.menubg));
      S.setProperty('--iup-menufg', rgb(c.menufg));
      S.setProperty('--iup-bd', rgb(mix(bg, edge, 0.35)));
      S.setProperty('--iup-bd-soft', rgb(mix(bg, edge, 0.19)));
      S.setProperty('--iup-bd-faint', rgb(mix(bg, edge, 0.10)));
      S.setProperty('--iup-bd-strong', rgb(mix(bg, edge, 0.44)));
      S.setProperty('--iup-bd-dark', rgb(mix(bg, edge, 0.64)));
      S.setProperty('--iup-track', rgb(mix(bg, edge, 0.05)));
      S.setProperty('--iup-face', rgb(mix(bg, W, dark ? 0.14 : 0.78)));
      S.setProperty('--iup-face2', rgb(mix(bg, W, dark ? 0.08 : 0.45)));
      S.setProperty('--iup-accent', rgb(ac));
      S.setProperty('--iup-accent-bg', rgb(mix(ac, dark ? bg : W, 0.78)));
      S.setProperty('--iup-accent-bg2', rgb(mix(ac, dark ? bg : W, 0.68)));
      if (!globalThis.__iupDarkWired && typeof window !== 'undefined' && window.matchMedia) {
        globalThis.__iupDarkWired = 1;
        try { window.matchMedia('(prefers-color-scheme: dark)').addEventListener('change', function () { D('iupwasmThemeChanged'); }); } catch (e) {}
      }
    }

    var apply = function (c) {
      var el = els[c.id];
      switch (c.op) {
      case 'theme': installTheme(c); break;
      case 'create': {
        el = document.createElement(c.tag);
        el.style.position = 'absolute'; el.style.boxSizing = 'border-box'; el.style.margin = '0';
        els[c.id] = el; el.dataset.iupId = c.id;
      } break;
      case 'destroy': {
        if (el && el.parentNode) el.parentNode.removeChild(el);
        delete els[c.id];
      } break;
      case 'addbody': {
        if (el) document.body.appendChild(el);
      } break;
      case 'addchild': {
        var p = els[c.pid]; if (p && el) p.appendChild(el);
      } break;
      case 'zorder': {
        if (el && el.parentNode) { if (c.top) el.parentNode.appendChild(el); else el.parentNode.insertBefore(el, el.parentNode.firstChild); }
      } break;
      case 'pos': {
        if (el) {
          var px = c.x, py = c.y;
          if (el.__iupDlg) {
            px = Math.max(0, Math.min(px, (window.innerWidth || c.w) - c.w));
            py = Math.max(0, Math.min(py, (window.innerHeight || c.h) - c.h));
          }
          el.style.left = px + 'px'; el.style.top = py + 'px'; el.style.width = c.w + 'px'; el.style.height = c.h + 'px';
          // a dialog sized to the viewport tracks future window resizes; a fixed-size one does not
          if (el.__iupDlg) el.__iupViewportFill = (c.w >= window.innerWidth && c.h >= window.innerHeight);
        }
      } break;
      case 'text': {
        if (el) el.textContent = c.text;
      } break;
      case 'visible': {
        if (el) {
          if (c.v) { if (el.style.display === 'none') el.style.display = el.__iupDisp || 'block'; }
          else { if (el.style.display !== 'none') el.__iupDisp = el.style.display; el.style.display = 'none'; }
        }
      } break;
      case 'active': {
        if (el) {
          el.__iupActive = !!c.a;
          el.style.pointerEvents = c.a ? 'auto' : 'none';
          if (el.__iupInput) el.__iupInput.disabled = !c.a;
          if (el.tagName === 'BUTTON' || el.tagName === 'INPUT') el.disabled = !c.a;
          // an explicit IMINACTIVE image shows at full opacity; otherwise dim the active image
          if (!c.a && el.__iupInactiveBg) { el.style.backgroundImage = el.__iupInactiveBg; el.style.opacity = '1'; }
          else if (!c.a && el.__iupInactiveImgSrc && el.__iupImg) { el.__iupImg.src = el.__iupInactiveImgSrc; el.style.opacity = '1'; }
          else if (c.a && el.__iupNormalBg) { el.style.backgroundImage = el.__iupNormalBg; el.style.opacity = '1'; }
          else if (c.a && el.__iupNormalImg && el.__iupImg) { el.__iupImg.src = el.__iupNormalImg; el.style.opacity = '1'; }
          else el.style.opacity = c.a ? '1' : '0.5';
        }
      } break;
      case 'iminactive': {
        if (el) {
          var iim = globalThis.__iupImg && globalThis.__iupImg.map[c.imgId];
          if (c.isBg) el.__iupInactiveBg = iim ? 'url(' + iim.url + ')' : null;
          else el.__iupInactiveImgSrc = iim ? iim.url : null;
          if (el.__iupActive === false) {
            if (c.isBg && el.__iupInactiveBg) { el.style.backgroundImage = el.__iupInactiveBg; el.style.opacity = '1'; }
            else if (!c.isBg && el.__iupInactiveImgSrc && el.__iupImg) { el.__iupImg.src = el.__iupInactiveImgSrc; el.style.opacity = '1'; }
          }
        }
      } break;
      case 'bgvar': {
        if (el) el.style.backgroundColor = c.css;
      } break;
      case 'fgvar': {
        if (el) el.style.color = c.css;
      } break;
      case 'bg': {
        if (el) el.style.backgroundColor = 'rgb(' + c.r + ',' + c.g + ',' + c.b + ')';
      } break;
      case 'fg': {
        if (el) el.style.color = 'rgb(' + c.r + ',' + c.g + ',' + c.b + ')';
      } break;
      case 'wire': {
        if (el) el.addEventListener('click', function () { D('iupwasmDispatchAction', c.id); });
      } break;
      case 'markup': {
        if (el) { var t = el.__iupLabel || el.__iupSpan || el; t.innerHTML = c.html; }
      } break;
      case 'btnstyle': {
        if (el) { el.style.display = 'flex'; el.style.alignItems = 'center'; el.style.justifyContent = 'center'; }
      } break;
      case 'btnalign': {
        if (el) el.style.justifyContent = c.a === 'left' ? 'flex-start' : c.a === 'right' ? 'flex-end' : 'center';
      } break;
      case 'btntext': {
        if (el) {
          if (!el.__iupLabel) {
            el.__iupLabel = document.createElement('span');
            el.__iupLabel.style.whiteSpace = 'nowrap'; el.__iupLabel.style.verticalAlign = 'middle';
            el.appendChild(el.__iupLabel);
          }
          el.__iupLabel.textContent = c.text;
        }
      } break;
      case 'btnimg': {
        if (el) {
          var bim = globalThis.__iupImg && globalThis.__iupImg.map[c.imgId];
          if (bim) {
            if (!el.__iupImg) {
              el.__iupImg = document.createElement('img');
              el.__iupImg.style.verticalAlign = 'middle'; el.style.whiteSpace = 'nowrap';
            }
            el.__iupImg.src = bim.url; el.__iupImg.width = bim.w; el.__iupImg.height = bim.h;
            el.__iupNormalImg = bim.url;
            if (el.__iupLabel && el.__iupLabel.textContent) el.__iupImg.style.marginRight = c.gap + 'px';
            if (el.firstChild) el.insertBefore(el.__iupImg, el.firstChild); else el.appendChild(el.__iupImg);
          }
        }
      } break;
      case 'btnimpress': {
        if (el) {
          var pim = globalThis.__iupImg && globalThis.__iupImg.map[c.imgId];
          el.__iupPressImg = pim ? pim.url : null;
          if (!el.__iupImpressWired) {
            el.__iupImpressWired = true;
            el.addEventListener('mousedown', function () { if (el.__iupImg && el.__iupPressImg) el.__iupImg.src = el.__iupPressImg; });
            var revert = function () { if (el.__iupImg && el.__iupNormalImg) el.__iupImg.src = el.__iupNormalImg; };
            el.addEventListener('mouseup', revert);
            el.addEventListener('mouseleave', revert);
          }
        }
      } break;
      case 'imagecreate': {
        if (!globalThis.__iupImg) globalThis.__iupImg = { map: {}, next: 1 };
        var icv = document.createElement('canvas');
        icv.width = c.w; icv.height = c.h;
        var ictx = icv.getContext('2d');
        var idata = ictx.createImageData(c.w, c.h);
        idata.data.set(c.rgba);
        ictx.putImageData(idata, 0, 0);
        var ient = globalThis.__iupImg.map[c.imgId] || {};
        ient.url = icv.toDataURL(); ient.w = c.w; ient.h = c.h; ient.canvas = icv;
        globalThis.__iupImg.map[c.imgId] = ient;
      } break;
      case 'imagedestroy': {
        if (globalThis.__iupImg) delete globalThis.__iupImg.map[c.imgId];
      } break;
      case 'setimage': {
        var sim = globalThis.__iupImg && globalThis.__iupImg.map[c.imgId];
        if (el && sim) {
          el.__iupNormalBg = 'url(' + sim.url + ')';
          el.style.backgroundImage = el.__iupNormalBg;
          el.style.backgroundRepeat = 'no-repeat'; el.style.backgroundPosition = el.style.backgroundPosition || 'center';
        }
      } break;
      case 'toggleimpress': {
        if (el) {
          var tpi = globalThis.__iupImg && globalThis.__iupImg.map[c.imgId];
          el.__iupPressBg = tpi ? 'url(' + tpi.url + ')' : null;
          if (!el.__iupTogImpressWired) {
            el.__iupTogImpressWired = true;
            el.addEventListener('mousedown', function () { if (el.__iupPressBg) el.style.backgroundImage = el.__iupPressBg; });
            var trevert = function () { if (el.__iupNormalBg) el.style.backgroundImage = el.__iupNormalBg; };
            el.addEventListener('mouseup', trevert);
            el.addEventListener('mouseleave', trevert);
          }
        }
      } break;
      case 'togglealign': {
        if (el) {
          var hx = c.h === 'ALEFT' ? 'left' : c.h === 'ARIGHT' ? 'right' : 'center';
          var vy = c.v === 'ATOP' ? 'top' : c.v === 'ABOTTOM' ? 'bottom' : 'center';
          el.style.backgroundPosition = hx + ' ' + vy;
        }
      } break;
      case 'togglerightbtn': {
        if (el && el.__iupInput) el.style.flexDirection = c.on ? 'row-reverse' : 'row';
      } break;
      case 'btnflat': {
        if (el) { el.style.border = 'none'; el.style.padding = '0'; el.style.background = 'transparent'; }
      } break;
      case 'btndefault': {
        if (el) el.style.boxShadow = c.on ? '0 0 0 1px var(--iup-accent)' : '';
      } break;
      case 'dlgstyle': {
        if (el) { el.style.background = 'var(--iup-bg)'; el.style.border = '1px solid var(--iup-bd-strong)'; el.style.overflow = 'hidden'; el.__iupDlg = 1; }
      } break;
      case 'dlgresize': {
        if (el) {
          if (c.on && !el.__iupResizeGrip) {
            var drid = c.id, grip = document.createElement('div');
            grip.style.cssText = 'position:absolute;right:0;bottom:0;width:14px;height:14px;cursor:nwse-resize;z-index:5;' +
              'background:linear-gradient(135deg,transparent 0 55%,var(--iup-bd-strong) 55% 62%,transparent 62% 70%,var(--iup-bd-strong) 70% 77%,transparent 77%);';
            el.appendChild(grip); el.__iupResizeGrip = grip;
            grip.addEventListener('mousedown', function (e) {
              e.preventDefault(); e.stopPropagation();
              var sx = e.clientX, sy = e.clientY, sw = el.offsetWidth, sh = el.offsetHeight;
              var mv = function (ev) {
                var nw = Math.max(40, sw + (ev.clientX - sx)), nh = Math.max(30, sh + (ev.clientY - sy));
                el.style.width = nw + 'px'; el.style.height = nh + 'px';
                D('iupwasmDialogResize', drid, nw, nh);
              };
              var up = function () { document.removeEventListener('mousemove', mv); document.removeEventListener('mouseup', up); };
              document.addEventListener('mousemove', mv); document.addEventListener('mouseup', up);
            });
          }
          if (el.__iupResizeGrip) el.__iupResizeGrip.style.display = c.on ? 'block' : 'none';
        }
      } break;
      case 'dlgbgimage': {
        if (el) {
          var dbi = globalThis.__iupImg && globalThis.__iupImg.map[c.imgId];
          if (dbi) {
            el.style.backgroundImage = 'url(' + dbi.url + ')';
            el.style.backgroundRepeat = c.zoom ? 'no-repeat' : 'repeat';
            el.style.backgroundSize = c.zoom ? '100% 100%' : 'auto';
          }
        }
      } break;
      case 'doctitle': {
        document.title = c.text;
      } break;
      case 'aria': {
        if (el) { if (c.text) el.setAttribute('aria-label', c.text); else el.removeAttribute('aria-label'); }
      } break;
      case 'ariadesc': {
        if (el) { if (c.text) el.setAttribute('aria-description', c.text); else el.removeAttribute('aria-description'); }
      } break;
      case 'setfont': {
        if (el) el.style.font = c.css;
      } break;
      case 'fontbase': {
        if (c.line) document.documentElement.style.setProperty('--iup-line', c.line + 'px');
        if (!globalThis.__iupFontBase) {
          globalThis.__iupFontBase = true;
          var fbs = document.createElement('style');
          fbs.textContent =
            'body{font:' + c.css + ';color:var(--iup-fg);}' +
            'button,input,select,textarea,optgroup,option,table,thead,tbody,tr,th,td{font:inherit;}' +
            // pin the line box to the measured charheight so VISIBLELINES is exact (no partial line)
            'input,textarea,select,option{line-height:var(--iup-line);}' +
            // default control colors from the theme; an explicit IUP BGCOLOR/FGCOLOR (inline) overrides
            'button{background:var(--iup-face);color:var(--iup-fg);border:1px solid var(--iup-bd);}' +
            'button:hover{background:var(--iup-face2);}' +
            'input,textarea,select{background:var(--iup-txtbg);color:var(--iup-txtfg);border:1px solid var(--iup-bd);}' +
            'option{background:var(--iup-txtbg);color:var(--iup-txtfg);padding-top:0;padding-bottom:0;}' +
            'input[type=number]::-webkit-inner-spin-button{opacity:1;height:auto;}' +
            '::-webkit-scrollbar{width:14px;height:14px;}' +
            '::-webkit-scrollbar-track{background:var(--iup-track);}' +
            '::-webkit-scrollbar-thumb{background:var(--iup-bd-soft);border:1px solid var(--iup-bd);}' +
            '::-webkit-scrollbar-thumb:hover{background:var(--iup-bd-soft);}' +
            '::-webkit-scrollbar-corner{background:var(--iup-track);}';
          document.head.appendChild(fbs);
        }
      } break;
      case 'focus': {
        if (el) { var ft = el.__iupInput || el.__iupVal || el; if (ft.focus) ft.focus(); }
      } break;
      case 'tip': {
        if (el) { if (c.text) el.title = c.text; else el.removeAttribute('title'); }
      } break;
      case 'padding': {
        if (el) el.style.padding = c.v + 'px ' + c.h + 'px';
      } break;
      case 'textalign': {
        if (el) el.style.textAlign = c.align;
      } break;
      case 'cursor': {
        if (el) el.style.cursor = c.css;
      } break;
      case 'labelstyle': {
        if (el) { el.style.whiteSpace = 'pre'; el.style.overflow = 'hidden'; }
      } break;
      case 'framestyle': {
        if (el) {
          el.style.border = 'none'; el.style.overflow = 'visible';
          var box = document.createElement('fieldset');
          box.style.position = 'absolute'; box.style.left = '0'; box.style.right = '0';
          box.style.top = '0'; box.style.bottom = '0';
          box.style.margin = '0'; box.style.padding = '0'; box.style.minInlineSize = '0';
          box.style.border = '1px solid var(--iup-bd)'; box.style.boxSizing = 'border-box';
          box.style.pointerEvents = 'none';
          el.insertBefore(box, el.firstChild); el.__iupBox = box;
        }
      } break;
      case 'framesunken': {
        if (el && el.__iupBox) el.__iupBox.style.borderStyle = c.on ? 'inset' : 'solid';
      } break;
      case 'frametitle': {
        if (el && el.__iupBox) {
          var ft = el.__iupTitle;
          if (!ft) {
            ft = document.createElement('legend');
            ft.style.margin = '0'; ft.style.marginLeft = '6px'; ft.style.padding = '0 4px';
            ft.style.width = 'auto'; ft.style.lineHeight = '1'; ft.style.whiteSpace = 'nowrap';
            el.__iupTitle = ft; el.__iupBox.insertBefore(ft, el.__iupBox.firstChild);
          }
          ft.textContent = c.text;
          ft.style.display = c.text ? '' : 'none';
        }
      } break;
      case 'createtoggle': {
        var lab = document.createElement('label');
        lab.style.position = 'absolute'; lab.style.boxSizing = 'border-box'; lab.style.whiteSpace = 'nowrap';
        lab.style.display = 'flex'; lab.style.alignItems = 'center';
        var inp = document.createElement('input');
        inp.type = c.isRadio ? 'radio' : 'checkbox';
        if (c.isRadio) {
          inp.name = 'iupradio' + c.group;
          if (document.getElementsByName(inp.name).length === 0) inp.checked = true;
        }
        var tsp = document.createElement('span'); tsp.style.marginLeft = '3px';
        lab.appendChild(inp); lab.appendChild(tsp);
        lab.__iupInput = inp; lab.__iupSpan = tsp; inp.__iupOwner = lab;
        els[c.id] = lab; lab.dataset.iupId = c.id;
      } break;
      case 'toggletext': {
        if (el && el.__iupSpan) el.__iupSpan.textContent = c.text;
      } break;
      case 'togglevalue': {
        if (el && el.__iupInput) {
          if (c.on === -1) { el.__iupInput.indeterminate = true; el.__iupInput.checked = false; el.__iupInput.__iup3Prev = -1; }
          else { el.__iupInput.indeterminate = false; el.__iupInput.checked = !!c.on; el.__iupInput.__iup3Prev = c.on ? 1 : 0; }
          if (el.__iupReflect) el.__iupReflect();
        }
      } break;
      case 'toggle3state': {
        if (el && el.__iupInput) el.__iup3State = !!c.on;
      } break;
      case 'toggleimg': {
        if (el && el.__iupInput) {
          el.__iupInput.style.display = 'none'; el.__iupSpan.style.display = 'none';
          el.style.justifyContent = 'center'; el.style.border = '1px solid var(--iup-bd-soft)';
          el.style.borderRadius = '2px'; el.style.cursor = 'pointer'; el.style.padding = '2px';
          el.__iupReflect = function () {
            var on = el.__iupInput.checked;
            el.style.backgroundColor = on ? 'var(--iup-accent-bg2)' : 'var(--iup-face2)';
            el.style.boxShadow = on ? 'inset 1px 1px 3px rgba(0,0,0,0.35)' : 'none';
            el.style.borderColor = on ? 'var(--iup-accent)' : 'var(--iup-bd-soft)';
          };
          el.__iupReflect();
        }
      } break;
      case 'toggleswitch': {
        if (el && el.__iupInput) {
          el.__iupInput.style.display = 'none'; el.__iupSpan.style.display = 'none';
          el.style.justifyContent = 'center'; el.style.cursor = 'pointer';
          var track = document.createElement('span');
          track.style.position = 'relative'; track.style.display = 'inline-block';
          track.style.width = '36px'; track.style.height = '18px'; track.style.borderRadius = '9px';
          track.style.transition = 'background-color .15s';
          var thumb = document.createElement('span');
          thumb.style.position = 'absolute'; thumb.style.top = '2px';
          thumb.style.width = '14px'; thumb.style.height = '14px'; thumb.style.borderRadius = '50%';
          thumb.style.background = 'var(--iup-txtbg)'; thumb.style.boxShadow = '0 1px 2px rgba(0,0,0,0.4)';
          thumb.style.transition = 'left .15s';
          track.appendChild(thumb); el.appendChild(track);
          el.__iupReflect = function () {
            var on = el.__iupInput.checked;
            track.style.background = on ? 'var(--iup-accent)' : 'var(--iup-bd-soft)';
            thumb.style.left = on ? '20px' : '2px';
          };
          el.__iupReflect();
        }
      } break;
      case 'togglewire': {
        if (el && el.__iupInput) {
          el.__iupInput.addEventListener('change', function () {
            var inp = el.__iupInput;
            var state;
            if (el.__iup3State) {
              var prev = inp.__iup3Prev || 0;
              state = (prev === 1) ? 0 : (prev === 0) ? -1 : 1;
              inp.indeterminate = (state === -1);
              inp.checked = (state === 1);
            } else {
              state = inp.checked ? 1 : 0;
            }
            // a radio unchecks the others without firing their change, so repaint the whole group
            if (inp.type === 'radio') {
              var grp = document.getElementsByName(inp.name);
              for (var i = 0; i < grp.length; i++) { var ow = grp[i].__iupOwner; if (ow && ow.__iupReflect) ow.__iupReflect(); }
            } else if (el.__iupReflect) el.__iupReflect();
            inp.__iup3Prev = state;
            D('iupwasmDispatchToggle', c.id, state);
          });
        }
      } break;
      case 'labelsep': {
        if (el) {
          if (c.horizontal) { el.style.borderTop = '1px solid var(--iup-bd)'; el.style.height = '1px'; }
          else { el.style.borderLeft = '1px solid var(--iup-bd)'; el.style.width = '1px'; }
        }
      } break;
      case 'labelwrap': {
        if (el) { el.style.whiteSpace = c.wrap ? 'normal' : 'pre'; if (c.wrap) el.style.overflowWrap = 'break-word'; }
      } break;
      case 'labelellipsis': {
        if (el) { el.style.whiteSpace = 'nowrap'; el.style.overflow = 'hidden'; el.style.textOverflow = c.on ? 'ellipsis' : 'clip'; }
      } break;
      case 'labelselect': {
        if (el) el.style.userSelect = c.on ? 'text' : 'none';
      } break;
      case 'favicon': {
        var fav = globalThis.__iupImg && globalThis.__iupImg.map[c.imgId];
        if (fav && fav.url) {
          var link = document.querySelector('link[rel~=icon]');
          if (!link) { link = document.createElement('link'); link.rel = 'icon'; document.head.appendChild(link); }
          link.href = fav.url;
        }
      } break;
      case 'fullscreen': {
        var fe = el || document.documentElement;
        if (c.on) { if (fe.requestFullscreen) fe.requestFullscreen(); }
        else { if (document.exitFullscreen && document.fullscreenElement) document.exitFullscreen(); }
      } break;
      case 'opacity': {
        if (el) el.style.opacity = c.opacity;
      } break;
      case 'bringfront': {
        if (el) { globalThis.__iupDlgZ = (globalThis.__iupDlgZ || 100) + 1; el.style.zIndex = globalThis.__iupDlgZ; }
      } break;
      case 'dlgmodal': {
        if (el) {
          if (!globalThis.__iupModalBackdrops) globalThis.__iupModalBackdrops = {};
          if (c.on) {
            globalThis.__iupModalZ = (globalThis.__iupModalZ || 9000) + 2;
            var bd = document.createElement('div');
            bd.style.cssText = 'position:fixed;inset:0;background:rgba(0,0,0,0.4);z-index:' + (globalThis.__iupModalZ - 1) + ';';
            document.body.appendChild(bd);
            globalThis.__iupModalBackdrops[c.id] = bd;
            el.style.zIndex = globalThis.__iupModalZ;
          } else {
            var prev = globalThis.__iupModalBackdrops[c.id];
            if (prev) { prev.remove(); delete globalThis.__iupModalBackdrops[c.id]; }
          }
        }
      } break;
      case 'createrange': {
        var rg = document.createElement('input');
        rg.type = 'range'; rg.min = '0'; rg.max = '1000'; rg.step = '1';
        rg.style.position = 'absolute'; rg.style.boxSizing = 'border-box'; rg.style.margin = '0';
        if (c.vertical) { rg.style.writingMode = 'vertical-lr'; rg.style.direction = 'rtl'; }
        els[c.id] = rg; rg.dataset.iupId = c.id;
        if (c.kind === 'scroll')
          rg.addEventListener('input', function () { D('iupwasmDispatchScrollbar', c.id, parseInt(rg.value)); });
      } break;
      case 'rangeset': {
        if (el) el.value = c.pos;
      } break;
      case 'valwire': {
        if (el) el.addEventListener('input', function () { D('iupwasmDispatchVal', c.id, parseInt(el.value)); });
      } break;
      case 'valstep': {
        if (el) el.step = c.step < 1 ? 1 : c.step;
      } break;
      case 'valpagestep': {
        if (el) {
          el.__iupPage = c.page < 1 ? 1 : c.page;
          if (!el.__iupPageWired) {
            el.__iupPageWired = true;
            el.addEventListener('keydown', function (e) {
              if (e.key !== 'PageUp' && e.key !== 'PageDown') return;
              e.preventDefault();
              var v = parseInt(el.value) + (e.key === 'PageUp' ? el.__iupPage : -el.__iupPage);
              v = Math.max(0, Math.min(1000, v));
              el.value = v; el.dispatchEvent(new Event('input'));
            });
          }
        }
      } break;
      case 'valticks': {
        if (el) {
          if (c.n <= 0) { el.removeAttribute('list'); }
          else {
            var lid = 'iupticks' + c.id;
            var dl = document.getElementById(lid);
            if (!dl) { dl = document.createElement('datalist'); dl.id = lid; document.body.appendChild(dl); }
            dl.innerHTML = '';
            for (var ti = 0; ti <= c.n; ti++) { var o = document.createElement('option'); o.value = Math.round(ti * 1000 / c.n); dl.appendChild(o); }
            el.setAttribute('list', lid);
          }
        }
      } break;
      case 'createprogress': {
        var ptrack = document.createElement('div');
        ptrack.style.position = 'absolute'; ptrack.style.boxSizing = 'border-box'; ptrack.style.overflow = 'hidden';
        ptrack.style.background = 'var(--iup-track)'; ptrack.style.border = '1px solid var(--iup-bd)';
        var pf = document.createElement('div');
        pf.style.position = 'absolute'; pf.style.background = 'var(--iup-accent)';
        if (c.vertical) { pf.style.left = '0'; pf.style.right = '0'; pf.style.bottom = '0'; pf.style.height = '0'; }
        else { pf.style.left = '0'; pf.style.top = '0'; pf.style.bottom = '0'; pf.style.width = '0'; }
        ptrack.appendChild(pf); ptrack.__iupFill = pf; ptrack.__iupVertical = c.vertical;
        els[c.id] = ptrack; ptrack.dataset.iupId = c.id;
      } break;
      case 'progressvalue': {
        if (el && !el.__iupMarquee) {
          var rr = (c.ratio < 0 ? 0 : c.ratio > 1 ? 1 : c.ratio) * 100 + '%';
          if (el.__iupVertical) el.__iupFill.style.height = rr; else el.__iupFill.style.width = rr;
        }
      } break;
      case 'progressmarquee': {
        if (el) {
          var mf = el.__iupFill; el.__iupMarquee = !!c.on;
          if (c.on) {
            var mvert = el.__iupVertical;
            if (mvert) { mf.style.height = '30%'; mf.style.width = '100%'; }
            else { mf.style.width = '30%'; mf.style.height = '100%'; }
            mf.animate(mvert ? [{ bottom: '-30%' }, { bottom: '100%' }] : [{ left: '-30%' }, { left: '100%' }], { duration: 1500, iterations: Infinity });
          }
        }
      } break;
      case 'progressapply': {
        if (el) {
          var paf = el.__iupFill, pac = el.__iupFg || 'var(--iup-accent)';
          if (el.__iupDashed) { var pang = el.__iupVertical ? '0deg' : '90deg'; paf.style.background = 'repeating-linear-gradient(' + pang + ', ' + pac + ' 0 12px, transparent 12px 16px)'; }
          else paf.style.background = pac;
        }
      } break;
      case 'progressfg': {
        if (el) el.__iupFg = c.fg;
      } break;
      case 'progressdashed': {
        if (el) el.__iupDashed = !!c.on;
      } break;
      case 'createdate': {
        var di = document.createElement('input');
        di.type = 'date'; di.style.position = 'absolute'; di.style.boxSizing = 'border-box'; di.style.margin = '0';
        els[c.id] = di; di.dataset.iupId = c.id;
        di.addEventListener('change', function () { D('iupwasmDispatchValueChanged', c.id); });
      } break;
      case 'inputval': {
        if (el) el.value = c.value;
      } break;
      case 'calstyle': {
        if (el) {
          el.style.background = 'var(--iup-bg)'; el.style.color = 'var(--iup-fg)';
          el.style.border = '1px solid var(--iup-bd)'; el.style.display = 'flex';
          el.style.flexDirection = 'column'; el.style.overflow = 'hidden'; el.style.userSelect = 'none';
          var calId = c.id;
          var head = document.createElement('div');
          head.style.display = 'flex'; head.style.alignItems = 'center'; head.style.justifyContent = 'space-between';
          head.style.padding = '2px 4px'; head.style.flex = '0 0 auto'; head.style.background = 'var(--iup-face2)';
          var mkNav = function (txt, delta) {
            var b = document.createElement('button');
            b.textContent = txt; b.style.padding = '0 8px'; b.style.cursor = 'pointer'; b.style.lineHeight = '1.4';
            b.addEventListener('click', function () { D('iupwasmCalendarNav', calId, delta); });
            return b;
          };
          var prev = mkNav(String.fromCharCode(0x25C0), -1);
          var next = mkNav(String.fromCharCode(0x25B6), 1);
          var lbl = document.createElement('span'); lbl.style.fontWeight = 'bold'; lbl.style.flex = '1 1 auto'; lbl.style.textAlign = 'center';
          head.appendChild(prev); head.appendChild(lbl); head.appendChild(next);
          var grid = document.createElement('div');
          grid.style.display = 'grid'; grid.style.flex = '1 1 auto'; grid.style.gap = '0';
          grid.style.gridTemplateRows = 'auto repeat(6, 1fr)';
          var wkdays = ['Su', 'Mo', 'Tu', 'We', 'Th', 'Fr', 'Sa'];
          el.__iupCalHead = []; el.__iupCalWeeks = []; el.__iupCalCells = [];
          var corner = document.createElement('div');
          corner.style.display = 'none'; corner.style.textAlign = 'center'; corner.style.fontSize = '11px';
          corner.style.color = 'var(--iup-bd-strong)'; corner.style.padding = '2px 0';
          grid.appendChild(corner); el.__iupCalCorner = corner;
          for (var wi = 0; wi < 7; wi++) {
            var wd = document.createElement('div');
            wd.textContent = wkdays[wi]; wd.style.textAlign = 'center'; wd.style.fontSize = '11px';
            wd.style.color = 'var(--iup-bd-strong)'; wd.style.padding = '2px 0';
            grid.appendChild(wd); el.__iupCalHead.push(wd);
          }
          for (var ri = 0; ri < 6; ri++) {
            var wk = document.createElement('div');
            wk.style.display = 'none'; wk.style.textAlign = 'center'; wk.style.fontSize = '11px';
            wk.style.color = 'var(--iup-bd-strong)'; wk.style.alignSelf = 'center';
            grid.appendChild(wk); el.__iupCalWeeks.push(wk);
            for (var ci = 0; ci < 7; ci++) {
              (function (idx) {
                var cell = document.createElement('div');
                cell.style.display = 'flex'; cell.style.alignItems = 'center'; cell.style.justifyContent = 'center';
                cell.style.cursor = 'pointer'; cell.style.borderRadius = '2px'; cell.style.margin = '1px';
                cell.addEventListener('click', function () {
                  if (cell.__iupY) D('iupwasmCalendarSelect', calId, cell.__iupY, cell.__iupM, cell.__iupD);
                });
                grid.appendChild(cell); el.__iupCalCells.push(cell);
              })(ri * 7 + ci);
            }
          }
          el.appendChild(head); el.appendChild(grid);
          el.__iupCalLbl = lbl; el.__iupCalGrid = grid;
        }
      } break;
      case 'calheader': {
        if (el && el.__iupCalLbl) {
          el.__iupCalLbl.textContent = c.label;
          var cols = c.weeknumbers ? '20px repeat(7, 1fr)' : 'repeat(7, 1fr)';
          el.__iupCalGrid.style.gridTemplateColumns = cols;
          el.__iupCalCorner.style.display = c.weeknumbers ? '' : 'none';
          for (var wj = 0; wj < el.__iupCalWeeks.length; wj++) el.__iupCalWeeks[wj].style.display = c.weeknumbers ? '' : 'none';
        }
      } break;
      case 'calcell': {
        if (el && el.__iupCalCells) {
          var cc = el.__iupCalCells[c.idx];
          if (cc) {
            cc.textContent = c.day; cc.__iupY = c.year; cc.__iupM = c.month; cc.__iupD = c.day;
            cc.style.color = c.outside ? 'var(--iup-bd-strong)' : 'var(--iup-fg)';
            cc.style.background = c.selected ? 'var(--iup-accent)' : '';
            cc.style.boxShadow = (c.today && !c.selected) ? 'inset 0 0 0 1px var(--iup-accent)' : '';
            cc.style.fontWeight = c.today ? 'bold' : 'normal';
            if (c.selected) cc.style.color = 'var(--iup-txtbg)';
          }
        }
      } break;
      case 'calweek': {
        if (el && el.__iupCalWeeks && el.__iupCalWeeks[c.row]) el.__iupCalWeeks[c.row].textContent = c.week;
      } break;
      case 'createpopover': {
        var pv = document.createElement('div');
        pv.style.position = 'fixed'; pv.style.boxSizing = 'border-box'; pv.style.display = 'none';
        pv.style.background = 'var(--iup-face)'; pv.style.border = '1px solid var(--iup-bd)';
        pv.style.boxShadow = '1px 2px 8px rgba(0,0,0,0.3)'; pv.style.zIndex = '2000';
        els[c.id] = pv; pv.dataset.iupId = c.id; document.body.appendChild(pv);
      } break;
      case 'popovershow': {
        if (el) {
          el.style.left = c.x + 'px'; el.style.top = c.y + 'px'; el.style.width = c.w + 'px'; el.style.height = c.h + 'px'; el.style.display = 'block';
          if (el.__iupArrow) { el.__iupArrow.remove(); el.__iupArrow = null; }
          if (c.arrow) {
            // popover side facing the anchor, derived from the placement enum (see iup_popover.h)
            var p = c.position, sz = 7, face = 'var(--iup-face)';
            var side = (p === 0 || p === 4 || p === 5) ? 'top' : (p === 1 || p === 6 || p === 7) ? 'bottom' : (p === 2 || p === 8 || p === 9) ? 'right' : 'left';
            var ar = document.createElement('div'); ar.style.position = 'absolute'; ar.style.width = '0'; ar.style.height = '0';
            if (side === 'top' || side === 'bottom') {
              ar.style.left = '50%'; ar.style.marginLeft = (-sz) + 'px';
              ar.style.borderLeft = sz + 'px solid transparent'; ar.style.borderRight = sz + 'px solid transparent';
              if (side === 'top') { ar.style.top = (-sz) + 'px'; ar.style.borderBottom = sz + 'px solid ' + face; }
              else { ar.style.bottom = (-sz) + 'px'; ar.style.borderTop = sz + 'px solid ' + face; }
            } else {
              ar.style.top = '50%'; ar.style.marginTop = (-sz) + 'px';
              ar.style.borderTop = sz + 'px solid transparent'; ar.style.borderBottom = sz + 'px solid transparent';
              if (side === 'left') { ar.style.left = (-sz) + 'px'; ar.style.borderRight = sz + 'px solid ' + face; }
              else { ar.style.right = (-sz) + 'px'; ar.style.borderLeft = sz + 'px solid ' + face; }
            }
            el.appendChild(ar); el.__iupArrow = ar;
          }
        }
      } break;
      case 'popoverhide': {
        if (el) el.style.display = 'none';
      } break;
      case 'popoverauto': {
        if (el) {
          if (c.install) {
            if (!el.__iupOutside) {
              var anchor = els[c.anchorId];
              el.__iupOutside = function (e) {
                if (el.contains(e.target)) return;
                if (anchor && anchor.contains(e.target)) return;
                D('iupwasmPopoverAutohide', c.id);
              };
              setTimeout(function () { document.addEventListener('mousedown', el.__iupOutside); }, 0);
            }
          } else if (el.__iupOutside) {
            document.removeEventListener('mousedown', el.__iupOutside);
            el.__iupOutside = null;
          }
        }
      } break;
      case 'menuinit': {
        if (!globalThis.__iupMenu) {
          globalThis.__iupMenu = { open: [], hover: 0 };
          globalThis.__iupMenuCloseAll = function () {
            var st = globalThis.__iupMenu.open;
            while (st.length) {
              var d = st.pop();
              if (d.style.display !== 'none') { d.style.display = 'none'; if (d.__iupId) D('iupwasmMenuState', d.__iupId, 0); }
            }
          };
          globalThis.__iupMenuShow = function (drop, x, y) {
            drop.style.left = x + 'px'; drop.style.top = y + 'px'; drop.style.display = 'block';
            if (globalThis.__iupMenu.open.indexOf(drop) < 0) globalThis.__iupMenu.open.push(drop);
            if (drop.__iupId) D('iupwasmMenuState', drop.__iupId, 1);
          };
          document.addEventListener('mousedown', function (e) {
            var st = globalThis.__iupMenu.open;
            if (!st.length) return;
            for (var i = 0; i < st.length; i++) if (st[i].contains(e.target)) return;
            if (e.target.closest && e.target.closest('[data-iup-menulabel]')) return;
            globalThis.__iupMenuCloseAll();
          });
          var M = globalThis.__iupMenu;
          var navItems = function (drop) {
            var out = [];
            for (var i = 0; i < drop.children.length; i++) {
              var ch = drop.children[i];
              if (ch.dataset && ch.dataset.iupMenulabel === '1' && ch.__iupActive !== false) out.push(ch);
            }
            return out;
          };
          var clearHi = function (drop) { var it = navItems(drop); for (var i = 0; i < it.length; i++) it[i].style.background = ''; };
          var setHi = function (drop, item) {
            clearHi(drop);
            drop.__iupHi = item || null;
            if (item) { item.style.background = 'var(--iup-accent-bg)'; var id = parseInt(item.dataset.iupId); M.hover = id; D('iupwasmMenuHighlight', id); }
          };
          var firstHi = function (drop) { var it = navItems(drop); if (it.length) setHi(drop, it[0]); };
          var moveHi = function (drop, dir) {
            var it = navItems(drop); if (!it.length) return;
            var cur = it.indexOf(drop.__iupHi);
            setHi(drop, it[cur < 0 ? (dir > 0 ? 0 : it.length - 1) : (cur + dir + it.length) % it.length]);
          };
          var openSub = function (drop) {
            var hi = drop.__iupHi;
            if (hi && hi.__iupDrop) {
              var r = hi.getBoundingClientRect();
              globalThis.__iupMenuShow(hi.__iupDrop, r.right, r.top);
              firstHi(hi.__iupDrop);
              return true;
            }
            return false;
          };
          var closeTop = function () {
            var st = M.open; if (!st.length) return;
            var d = st.pop(); clearHi(d); d.__iupHi = null;
            d.style.display = 'none'; if (d.__iupId) D('iupwasmMenuState', d.__iupId, 0);
          };
          var barLabels = function (lab) {
            var bar = lab && lab.parentNode, out = [];
            if (bar) for (var i = 0; i < bar.children.length; i++) if (bar.children[i].__iupDrop) out.push(bar.children[i]);
            return out;
          };
          var barMove = function (dir) {
            if (!M.open.length) return;
            var lab = M.open[0].__iupLabel, labs = barLabels(lab), ci = labs.indexOf(lab);
            if (ci < 0) return;
            var nl = labs[(ci + dir + labs.length) % labs.length];
            globalThis.__iupMenuCloseAll();
            var r = nl.getBoundingClientRect();
            globalThis.__iupMenuShow(nl.__iupDrop, r.left, r.bottom);
            firstHi(nl.__iupDrop);
          };
          var mnemHit = function (drop, ch) {
            var it = navItems(drop); ch = ch.toLowerCase();
            for (var i = 0; i < it.length; i++) if (it[i].__iupMnem === ch) return it[i];
            return null;
          };
          var barOpenMnem = function (ch) {
            ch = ch.toLowerCase();
            var bars = document.querySelectorAll('[data-iup-menubar="1"]');
            for (var b = 0; b < bars.length; b++) {
              var kids = bars[b].children;
              for (var i = 0; i < kids.length; i++)
                if (kids[i].__iupDrop && kids[i].__iupMnem === ch) {
                  var r = kids[i].getBoundingClientRect();
                  globalThis.__iupMenuShow(kids[i].__iupDrop, r.left, r.bottom);
                  firstHi(kids[i].__iupDrop);
                  return true;
                }
            }
            return false;
          };
          document.addEventListener('keydown', function (e) {
            if (!M.open.length) {
              if (e.altKey && !e.ctrlKey && e.key.length === 1 && barOpenMnem(e.key)) e.preventDefault();
              return;
            }
            var drop = M.open[M.open.length - 1], k = e.key;
            if (k === 'F1') { if (M.hover) { e.preventDefault(); D('iupwasmMenuHelp', M.hover); } return; }
            if (k === 'Escape') { e.preventDefault(); closeTop(); return; }
            if (k === 'ArrowDown') { e.preventDefault(); moveHi(drop, 1); return; }
            if (k === 'ArrowUp') { e.preventDefault(); moveHi(drop, -1); return; }
            if (k === 'Home') { e.preventDefault(); firstHi(drop); return; }
            if (k === 'End') { e.preventDefault(); var t = navItems(drop); if (t.length) setHi(drop, t[t.length - 1]); return; }
            if (k === 'ArrowRight') { e.preventDefault(); if (!openSub(drop)) barMove(1); return; }
            if (k === 'ArrowLeft') { e.preventDefault(); if (M.open.length > 1) closeTop(); else barMove(-1); return; }
            if (k === 'Enter' || k === ' ') {
              e.preventDefault();
              var hi = drop.__iupHi; if (!hi) return;
              if (hi.__iupDrop) openSub(drop); else hi.click();
              return;
            }
            if (k.length === 1) {
              var hit = mnemHit(drop, k);
              if (hit) { e.preventDefault(); setHi(drop, hit); if (hit.__iupDrop) openSub(drop); else hit.click(); }
            }
          });
        }
      } break;
      case 'createmenubar': {
        var mb = document.createElement('div');
        mb.style.position = 'absolute'; mb.style.left = '0'; mb.style.top = '0'; mb.style.right = '0';
        mb.style.height = '24px'; mb.style.display = 'flex'; mb.style.alignItems = 'stretch';
        mb.style.background = 'var(--iup-face2)'; mb.style.borderBottom = '1px solid var(--iup-bd-soft)'; mb.style.zIndex = '10';
        els[c.id] = mb; mb.dataset.iupId = c.id; mb.dataset.iupMenubar = '1';
      } break;
      case 'createdropdown': {
        var dd = document.createElement('div');
        dd.style.position = 'fixed'; dd.style.display = 'none'; dd.style.background = 'var(--iup-face)';
        dd.style.border = '1px solid var(--iup-bd-soft)'; dd.style.boxShadow = '1px 2px 6px rgba(0,0,0,0.25)';
        dd.style.padding = '2px 0'; dd.style.minWidth = '140px'; dd.style.zIndex = '1000';
        els[c.id] = dd; dd.dataset.iupId = c.id; dd.__iupId = c.id; document.body.appendChild(dd);
      } break;
      case 'createmenuentry': {
        var me = document.createElement('div');
        els[c.id] = me; me.dataset.iupId = c.id;
        if (c.kind === 2) {
          me.style.height = '0'; me.style.borderTop = '1px solid var(--iup-bd-faint)'; me.style.margin = '3px 0';
        } else {
          me.dataset.iupMenulabel = '1';
          me.style.padding = '3px 22px 3px 22px'; me.style.whiteSpace = 'nowrap'; me.style.cursor = 'default'; me.style.position = 'relative';
          me.addEventListener('mouseenter', function () { me.style.background = 'var(--iup-accent-bg)'; });
          me.addEventListener('mouseleave', function () { me.style.background = ''; });
          if (c.kind === 1) {
            var arrow = document.createElement('span');
            arrow.textContent = String.fromCharCode(0x25B6);
            arrow.style.position = 'absolute'; arrow.style.right = '6px'; arrow.style.fontSize = '9px';
            me.appendChild(arrow); me.__iupArrow = arrow;
          }
          var msp = document.createElement('span');
          me.insertBefore(msp, me.firstChild); me.__iupSpan = msp;
          var chk = document.createElement('span');
          chk.style.position = 'absolute'; chk.style.left = '6px';
          me.appendChild(chk); me.__iupCheck = chk;
        }
      } break;
      case 'menutext': {
        if (el && el.__iupSpan) {
          var sp = el.__iupSpan, txt = c.text, mn = (c.mnem || '').toLowerCase();
          el.__iupMnem = mn;
          var mi = mn ? txt.toLowerCase().indexOf(mn) : -1;
          if (mi >= 0) {
            sp.textContent = '';
            sp.appendChild(document.createTextNode(txt.slice(0, mi)));
            var mu = document.createElement('u'); mu.textContent = txt.charAt(mi); sp.appendChild(mu);
            sp.appendChild(document.createTextNode(txt.slice(mi + 1)));
          } else sp.textContent = txt;
          if (c.accel) {
            if (!el.__iupAccel) {
              el.__iupAccel = document.createElement('span');
              el.__iupAccel.style.marginLeft = '24px'; el.__iupAccel.style.opacity = '0.6'; el.__iupAccel.style.fontSize = '0.92em';
              el.style.display = 'flex'; el.style.alignItems = 'center'; el.style.justifyContent = 'space-between';
              if (el.__iupArrow) el.insertBefore(el.__iupAccel, el.__iupArrow); else el.appendChild(el.__iupAccel);
            }
            el.__iupAccel.textContent = c.accel;
          } else if (el.__iupAccel) el.__iupAccel.textContent = '';
        }
      } break;
      case 'menucolor': {
        if (el) { if (c.isBg) el.style.background = c.css; else el.style.color = c.css; }
      } break;
      case 'menucheck': {
        if (el && el.__iupCheck && !el.__iupHasGutterImg) el.__iupCheck.textContent = c.on ? String.fromCharCode(0x2713) : '';
      } break;
      case 'menuhidemark': {
        if (el && el.__iupCheck) { el.__iupCheck.style.display = c.hide ? 'none' : ''; if (c.hide) el.__iupCheck.textContent = ''; }
      } break;
      case 'menuimage': {
        if (el) {
          var mim = globalThis.__iupImg && globalThis.__iupImg.map[c.imgId];
          if (mim) {
            if (!el.__iupImg) {
              el.__iupImg = document.createElement('img');
              el.__iupImg.style.position = 'absolute'; el.__iupImg.style.left = '3px';
              el.__iupImg.style.top = '50%'; el.__iupImg.style.transform = 'translateY(-50%)';
              el.appendChild(el.__iupImg);
            }
            el.__iupImg.src = mim.url; el.__iupImg.width = mim.w; el.__iupImg.height = mim.h;
            el.__iupImg.style.display = '';
            // IUP IMAGE/IMPRESS are the check-mark images: gutter image replaces the check glyph
            el.__iupHasGutterImg = true;
            if (el.__iupCheck) el.__iupCheck.textContent = '';
          } else if (el.__iupImg) { el.__iupImg.style.display = 'none'; el.__iupHasGutterImg = false; }
        }
      } break;
      case 'menuinsert': {
        var mp = els[c.pid], mc = els[c.cid];
        if (mp && mc) { var mref = mp.children[c.pos]; if (mref) mp.insertBefore(mc, mref); else mp.appendChild(mc); }
      } break;
      case 'menuitemwire': {
        if (el) {
          el.addEventListener('click', function (e) { e.stopPropagation(); globalThis.__iupMenuCloseAll(); D('iupwasmDispatchAction', c.id); });
          el.addEventListener('mouseenter', function () { globalThis.__iupMenu.hover = c.id; D('iupwasmMenuHighlight', c.id); });
          el.addEventListener('mouseleave', function () { if (globalThis.__iupMenu.hover === c.id) globalThis.__iupMenu.hover = 0; });
        }
      } break;
      case 'recentwire': {
        if (el) el.addEventListener('click', function (e) { e.stopPropagation(); globalThis.__iupMenuCloseAll(); D('iupwasmRecentActivate', c.menuId, c.index); });
      } break;
      case 'submenuattach': {
        var lab = els[c.labelId], drop = els[c.dropId];
        if (lab && drop) {
          lab.__iupDrop = drop;
          drop.__iupLabel = lab;
          var tl = c.topLevel;
          var openAt = function () {
            var r = lab.getBoundingClientRect();
            if (tl) globalThis.__iupMenuShow(drop, r.left, r.bottom);
            else globalThis.__iupMenuShow(drop, r.right, r.top);
          };
          if (tl) {
            if (lab.__iupArrow) lab.__iupArrow.style.display = 'none';
            lab.style.display = 'flex'; lab.style.alignItems = 'center'; lab.style.padding = '0 10px';
            lab.addEventListener('click', function (e) {
              e.stopPropagation();
              var wasOpen = drop.style.display !== 'none';
              globalThis.__iupMenuCloseAll();
              if (!wasOpen) openAt();
            });
          } else {
            lab.addEventListener('mouseenter', function () {
              var st = globalThis.__iupMenu.open;
              while (st.length && st[st.length - 1] !== drop && !st[st.length - 1].contains(lab)) {
                var top = st[st.length - 1];
                if (top === lab.parentNode) break;
                st.pop(); top.style.display = 'none';
                if (top.__iupId) D('iupwasmMenuState', top.__iupId, 0);
              }
              openAt();
            });
          }
        }
      } break;
      case 'menupopup': {
        var mpd = els[c.dropId];
        if (mpd) {
          globalThis.__iupMenuShow(mpd, c.x, c.y);
          if (c.align) {
            var pa = c.align.split(':'), pax = c.x, pay = c.y;
            var ph = (pa[0] || '').toUpperCase(), pv = (pa[1] || '').toUpperCase();
            if (ph === 'ARIGHT') pax -= mpd.offsetWidth; else if (ph === 'ACENTER') pax -= mpd.offsetWidth / 2;
            if (pv === 'ABOTTOM') pay -= mpd.offsetHeight; else if (pv === 'ACENTER') pay -= mpd.offsetHeight / 2;
            mpd.style.left = pax + 'px'; mpd.style.top = pay + 'px';
          }
        }
      } break;
      case 'createtabs': {
        var wrap = document.createElement('div');
        wrap.style.position = 'absolute'; wrap.style.boxSizing = 'border-box'; wrap.style.display = 'flex'; wrap.style.flexDirection = 'column';
        var strip = document.createElement('div');
        strip.style.display = 'flex'; strip.style.flexWrap = 'nowrap'; strip.style.flex = '0 0 auto'; strip.style.borderBottom = '1px solid var(--iup-bd)';
        var content = document.createElement('div');
        content.style.position = 'relative'; content.style.flex = '1 1 auto'; content.style.overflow = 'hidden';
        wrap.appendChild(strip); wrap.appendChild(content);
        wrap.__iupStrip = strip; wrap.__iupContent = content; wrap.__iupPages = []; wrap.__iupTabs = []; wrap.__iupCurrent = -1;
        els[c.id] = wrap; wrap.dataset.iupId = c.id;
      } break;
      case 'tabsconfig': {
        var tw = els[c.id];
        if (tw) {
          tw.__iupShowClose = !!c.showClose; tw.__iupReorder = !!c.reorder;
          for (var i = 0; i < tw.__iupTabs.length; i++) {
            var tcb = tw.__iupTabs[i];
            if (tcb.__iupClose) tcb.__iupClose.style.display = c.showClose ? '' : 'none';
            tcb.draggable = !!c.reorder;
          }
        }
      } break;
      case 'tabsaddpage': {
        var wrap = els[c.id];
        if (wrap) {
          var tabsId = c.id;
          var page = document.createElement('div');
          page.style.position = 'absolute'; page.style.left = '0'; page.style.top = '0'; page.style.right = '0'; page.style.bottom = '0'; page.style.display = 'none';
          wrap.__iupContent.appendChild(page);
          els[c.pid] = page; page.dataset.iupId = c.pid;
          var tab = document.createElement('button');
          tab.style.padding = '2px 10px'; tab.style.border = '1px solid var(--iup-bd)'; tab.style.background = 'var(--iup-bd-faint)';
          tab.style.display = 'flex'; tab.style.alignItems = 'center'; tab.style.gap = '4px';
          tab.style.flexShrink = '0'; tab.style.whiteSpace = 'nowrap';
          if (wrap.__iupPadH != null) tab.style.padding = (2 + wrap.__iupPadV) + 'px ' + (10 + wrap.__iupPadH) + 'px';
          var timg = document.createElement('img'); timg.style.display = 'none';
          var tlbl = document.createElement('span'); tlbl.style.whiteSpace = 'nowrap'; tlbl.textContent = c.title;
          tab.appendChild(timg); tab.appendChild(tlbl); tab.__iupImg = timg; tab.__iupLbl = tlbl;
          var curPos = function () { return wrap.__iupTabs.indexOf(tab); };
          tab.addEventListener('click', function () {
            var pos = curPos(), old = wrap.__iupCurrent;
            D('iupwasmTabsSetCurrent', tabsId, pos);
            if (old !== pos) D('iupwasmDispatchTabChange', tabsId, pos, old);
          });
          tab.addEventListener('contextmenu', function (e) { e.preventDefault(); D('iupwasmTabsRightClick', tabsId, curPos()); });
          var tcl = document.createElement('span');
          tcl.textContent = String.fromCharCode(0xD7);
          tcl.style.cssText = 'margin-left:6px;color:var(--iup-bd-strong);font-weight:bold;line-height:1;';
          tcl.style.display = wrap.__iupShowClose ? '' : 'none';
          tcl.addEventListener('mousedown', function (e) { e.stopPropagation(); e.preventDefault(); D('iupwasmTabsClose', tabsId, curPos()); });
          tab.appendChild(tcl); tab.__iupClose = tcl;
          tab.draggable = !!wrap.__iupReorder;
          tab.addEventListener('dragstart', function (e) { wrap.__iupDragFrom = curPos(); if (e.dataTransfer) e.dataTransfer.effectAllowed = 'move'; });
          tab.addEventListener('dragover', function (e) { if (!wrap.__iupReorder) return; e.preventDefault(); if (e.dataTransfer) e.dataTransfer.dropEffect = 'move'; });
          tab.addEventListener('drop', function (e) { if (!wrap.__iupReorder) return; e.preventDefault(); var from = wrap.__iupDragFrom, to = curPos(); if (from != null && from !== to) D('iupwasmTabsReorder', tabsId, from, to); });
          if (wrap.__iupTabFg) tlbl.style.color = wrap.__iupTabFg;
          if (wrap.__iupPageBg) page.style.background = wrap.__iupPageBg;
          wrap.__iupStrip.appendChild(tab);
          wrap.__iupPages.push(page); wrap.__iupTabs.push(tab);
          // select first tab synchronously, else the add burst leaves __iupCurrent at -1 and the last tab wins
          if (wrap.__iupTabs.length === 1) { wrap.__iupCurrent = 0; page.style.display = 'block'; tab.style.background = 'var(--iup-face2)'; }
        }
      } break;
      case 'tabssetcurrent': {
        var tw = els[c.id];
        if (tw) {
          for (var i = 0; i < tw.__iupPages.length; i++) {
            tw.__iupPages[i].style.display = (i === c.pos) ? 'block' : 'none';
            tw.__iupTabs[i].style.background = (i === c.pos) ? 'var(--iup-face2)' : 'var(--iup-bd-faint)';
          }
          tw.__iupCurrent = c.pos;
        }
      } break;
      case 'tabspadding': {
        var tw = els[c.id];
        if (tw) {
          tw.__iupPadH = c.h; tw.__iupPadV = c.v;
          for (var i = 0; i < tw.__iupTabs.length; i++)
            tw.__iupTabs[i].style.padding = (2 + c.v) + 'px ' + (10 + c.h) + 'px';
        }
      } break;
      case 'tabssettip': {
        var tw = els[c.id];
        if (tw && tw.__iupTabs[c.pos]) tw.__iupTabs[c.pos].title = c.tip;
      } break;
      case 'tabshide': {
        var tw = els[c.id];
        if (tw && tw.__iupTabs[c.pos]) {
          tw.__iupTabs[c.pos].style.display = 'none';
          if (c.pos === tw.__iupCurrent) {
            for (var i = 0; i < tw.__iupTabs.length; i++)
              if (i !== c.pos && tw.__iupTabs[i].style.display !== 'none') { D('iupwasmTabsSetCurrent', c.id, i); break; }
          }
        }
      } break;
      case 'tabsremove': {
        var tw = els[c.id];
        if (tw && c.pos >= 0 && c.pos < tw.__iupTabs.length) {
          var rt = tw.__iupTabs.splice(c.pos, 1)[0];
          var rp = tw.__iupPages.splice(c.pos, 1)[0];
          if (rt) rt.remove();
          if (rp) rp.remove();
          if (tw.__iupCurrent >= tw.__iupTabs.length) tw.__iupCurrent = tw.__iupTabs.length - 1;
          if (tw.__iupCurrent >= 0) D('iupwasmTabsSetCurrent', c.id, tw.__iupCurrent);
        }
      } break;
      case 'tabsreorderdom': {
        var tw = els[c.id];
        if (tw) {
          var rtabs = tw.__iupTabs, rpages = tw.__iupPages, rstrip = tw.__iupStrip;
          var curTab = rtabs[tw.__iupCurrent];
          var rt2 = rtabs.splice(c.from, 1)[0], rpg = rpages.splice(c.from, 1)[0];
          rtabs.splice(c.to, 0, rt2); rpages.splice(c.to, 0, rpg);
          for (var i = 0; i < rtabs.length; i++) rstrip.appendChild(rtabs[i]);
          tw.__iupCurrent = rtabs.indexOf(curTab);
        }
      } break;
      case 'tabssettitle': {
        var tw = els[c.id];
        if (tw && tw.__iupTabs[c.pos]) tw.__iupTabs[c.pos].__iupLbl.textContent = c.title;
      } break;
      case 'tabssetimage': {
        var tw = els[c.id];
        if (tw && tw.__iupTabs[c.pos]) {
          var tim = tw.__iupTabs[c.pos].__iupImg;
          var xim = globalThis.__iupImg && globalThis.__iupImg.map[c.imgId];
          if (xim) {
            tim.src = xim.url;
            if (c.w > 0 && c.h > 0) { tim.width = c.w; tim.height = c.h; }
            else { tim.removeAttribute('width'); tim.removeAttribute('height'); }
            tim.style.display = 'inline';
          } else tim.style.display = 'none';
        }
      } break;
      case 'tabssetvisible': {
        var tw = els[c.id];
        if (tw && tw.__iupTabs[c.pos]) tw.__iupTabs[c.pos].style.display = c.on ? 'flex' : 'none';
      } break;
      case 'tabssettype': {
        var tw = els[c.id];
        if (tw) {
          var tstrip = tw.__iupStrip; tstrip.style.border = 'none';
          if (c.type === 1) { tw.style.flexDirection = 'column-reverse'; tstrip.style.flexDirection = 'row'; tstrip.style.flexWrap = 'nowrap'; tstrip.style.borderTop = '1px solid var(--iup-bd)'; }
          else if (c.type === 2) { tw.style.flexDirection = 'row'; tstrip.style.flexDirection = 'column'; tstrip.style.flexWrap = 'nowrap'; tstrip.style.borderRight = '1px solid var(--iup-bd)'; }
          else if (c.type === 3) { tw.style.flexDirection = 'row-reverse'; tstrip.style.flexDirection = 'column'; tstrip.style.flexWrap = 'nowrap'; tstrip.style.borderLeft = '1px solid var(--iup-bd)'; }
          else { tw.style.flexDirection = 'column'; tstrip.style.flexDirection = 'row'; tstrip.style.flexWrap = 'nowrap'; tstrip.style.borderBottom = '1px solid var(--iup-bd)'; }
        }
      } break;
      case 'tabssetvertical': {
        var tw = els[c.id];
        if (tw) for (var i = 0; i < tw.__iupTabs.length; i++) {
          var vlbl = tw.__iupTabs[i].__iupLbl;
          vlbl.style.writingMode = c.vertical ? 'vertical-rl' : '';
          vlbl.style.transform = c.vertical ? 'rotate(180deg)' : '';
        }
      } break;
      case 'tabsfgcolor': {
        var tw = els[c.id];
        if (tw) {
          tw.__iupTabFg = c.css;
          for (var i = 0; i < tw.__iupTabs.length; i++) tw.__iupTabs[i].__iupLbl.style.color = c.css;
        }
      } break;
      case 'tabsbgcolor': {
        var tw = els[c.id];
        if (tw) {
          tw.__iupPageBg = c.css;
          for (var i = 0; i < tw.__iupPages.length; i++) tw.__iupPages[i].style.background = c.css;
        }
      } break;
      case 'tabsshowclose': {
        var tw = els[c.id];
        if (tw && tw.__iupTabs[c.pos] && tw.__iupTabs[c.pos].__iupClose)
          tw.__iupTabs[c.pos].__iupClose.style.display = c.on ? '' : 'none';
      } break;
      case 'createtext': {
        var tel;
        if (c.multiline && c.formatting) {
          tel = document.createElement('div');
          tel.contentEditable = 'true';
          tel.style.whiteSpace = c.wordwrap ? 'pre-wrap' : 'pre'; tel.style.overflow = 'auto'; tel.style.font = 'inherit';
          tel.style.background = 'var(--iup-txtbg)'; tel.style.border = '1px solid var(--iup-bd)';
          tel.__iupText = ''; tel.__iupTags = [];
          tel.__iupRender = function () {
            var t = tel.__iupText, tags = tel.__iupTags;
            if (!tags.length) { tel.textContent = t; return; }
            function esc(ch) { return ch === '<' ? '&lt;' : ch === '>' ? '&gt;' : ch === '&' ? '&amp;' : ch; }
            function styleAt(p) { var a = ''; for (var k = 0; k < tags.length; k++) { var g = tags[k]; if (g.css && p >= g.s && p < g.e) a += g.css; } return a; }
            function imgAt(p) { for (var k = 0; k < tags.length; k++) { var g = tags[k]; if (g.img && g.s === p) return g; } return null; }
            function alignIn(a, b) { for (var k = 0; k < tags.length; k++) { var g = tags[k]; if (g.align && g.s < b && g.e > a) return g.align; } return ''; }
            var lines = t.split('\n'), html = '', pos = 0;
            for (var li = 0; li < lines.length; li++) {
              var line = lines[li], ls = pos, le = pos + line.length;
              var al = alignIn(ls, le > ls ? le : ls + 1);
              var inner = '', buf = '', cur = null, i = 0;
              while (i < line.length) {
                var gp = ls + i, im = imgAt(gp);
                if (im) {
                  if (buf) inner += cur ? '<span style="' + cur + '">' + buf + '</span>' : buf;
                  buf = ''; cur = null;
                  var dim = ''; if (im.iw) dim += 'width:' + im.iw + 'px;'; if (im.ih) dim += 'height:' + im.ih + 'px;';
                  inner += '<img src="' + im.img + '" style="vertical-align:middle;' + dim + '">';
                  var step = im.e - gp; i += step > 0 ? step : 1;
                  continue;
                }
                var st = styleAt(gp);
                if (st !== cur) { if (buf) inner += cur ? '<span style="' + cur + '">' + buf + '</span>' : buf; buf = ''; cur = st; }
                buf += esc(line[i]); i++;
              }
              if (buf) inner += cur ? '<span style="' + cur + '">' + buf + '</span>' : buf;
              if (!inner) inner = '<br>';
              html += '<div' + (al ? ' style="text-align:' + al + '"' : '') + '>' + inner + '</div>';
              pos = le + 1;
            }
            tel.innerHTML = html;
          };
          if (!globalThis.__iupTextOff) globalThis.__iupTextOff = function (elem, lin, col) {
            var lines = elem.__iupText.split('\n'), o = 0, k;
            for (k = 0; k < lin - 1 && k < lines.length; k++) o += lines[k].length + 1;
            var ll = (lin - 1 < lines.length) ? lines[lin - 1].length : 0;
            if (col - 1 > ll) col = ll + 1;
            return o + (col - 1);
          };
          Object.defineProperty(tel, 'value', {
            get: function () { return tel.__iupText; },
            set: function (v) { tel.__iupText = v || ''; tel.__iupTags = []; tel.__iupRender(); },
            configurable: true
          });
        } else if (c.multiline) { tel = document.createElement('textarea'); tel.wrap = c.wordwrap ? 'soft' : 'off'; }
        else { tel = document.createElement('input'); tel.type = c.spin ? 'number' : (c.password ? 'password' : 'text'); }
        tel.style.position = 'absolute'; tel.style.boxSizing = 'border-box'; tel.style.margin = '0';
        els[c.id] = tel; tel.dataset.iupId = c.id;
      } break;
      case 'textaddtag': {
        if (el && el.__iupRender) {
          var tlines = el.__iupText.split('\n');
          var toff = function (lin, col) {
            var o = 0, k;
            for (k = 0; k < lin - 1 && k < tlines.length; k++) o += tlines[k].length + 1;
            var ll = (lin - 1 < tlines.length) ? tlines[lin - 1].length : 0;
            if (col - 1 > ll) col = ll + 1;
            return o + (col - 1);
          };
          var ts = toff(c.l1, c.c1), te = toff(c.l2, c.c2);
          if (te > el.__iupText.length) te = el.__iupText.length;
          if (te > ts) { el.__iupTags.push({ s: ts, e: te, css: c.css }); el.__iupRender(); }
        }
      } break;
      case 'textaddtagpos': {
        if (el && el.__iupRender) {
          var te2 = c.end; if (te2 > el.__iupText.length) te2 = el.__iupText.length;
          if (te2 > c.start && c.start >= 0) { el.__iupTags.push({ s: c.start, e: te2, css: c.css }); el.__iupRender(); }
        }
      } break;
      case 'textaddimagetag': {
        if (el && el.__iupRender) {
          var xim2 = globalThis.__iupImg && globalThis.__iupImg.map[c.imgId];
          if (xim2) {
            var is = globalThis.__iupTextOff(el, c.l1, c.c1), ie = globalThis.__iupTextOff(el, c.l2, c.c2);
            if (ie > el.__iupText.length) ie = el.__iupText.length;
            if (ie > is) { el.__iupTags.push({ s: is, e: ie, img: xim2.url, iw: c.w, ih: c.h }); el.__iupRender(); }
          }
        }
      } break;
      case 'textaddimagetagpos': {
        if (el && el.__iupRender) {
          var xim3 = globalThis.__iupImg && globalThis.__iupImg.map[c.imgId];
          if (xim3) {
            var ie2 = c.end; if (ie2 > el.__iupText.length) ie2 = el.__iupText.length;
            if (ie2 > c.start && c.start >= 0) { el.__iupTags.push({ s: c.start, e: ie2, img: xim3.url, iw: c.w, ih: c.h }); el.__iupRender(); }
          }
        }
      } break;
      case 'textaddalign': {
        if (el && el.__iupRender) {
          var as = globalThis.__iupTextOff(el, c.l1, c.c1), ae = globalThis.__iupTextOff(el, c.l2, c.c2);
          el.__iupTags.push({ s: as, e: ae, align: c.align }); el.__iupRender();
        }
      } break;
      case 'textaddalignpos': {
        if (el && el.__iupRender) { el.__iupTags.push({ s: c.start, e: c.end, align: c.align }); el.__iupRender(); }
      } break;
      case 'textspinrange': {
        if (el) { el.min = c.min; el.max = c.max; el.step = c.step < 1 ? 1 : c.step; }
      } break;
      case 'textspinwire': {
        if (el) {
          var sid = c.id;
          el.__iupSpinPrev = parseFloat(el.value) || 0;
          el.addEventListener('input', function () {
            var v = parseFloat(el.value);
            if (!isNaN(v) && v !== el.__iupSpinPrev) { D('iupwasmDispatchSpin', sid, v > el.__iupSpinPrev ? 1 : -1); el.__iupSpinPrev = v; }
          });
        }
      } break;
      case 'textsetvalue': {
        if (el) el.value = c.value;
      } break;
      case 'textappend': {
        if (el) {
          if (el.__iupRender) {
            if (el.__iupText.length > 0) el.__iupText += '\n';
            el.__iupText += c.value; el.__iupRender();
          } else {
            if (c.multiline && el.value.length > 0) el.value += '\n';
            el.value += c.value;
          }
          el.scrollTop = el.scrollHeight;
        }
      } break;
      case 'textreadonly': {
        if (el) { if (el.__iupRender) el.contentEditable = c.ro ? 'false' : 'true'; else el.readOnly = !!c.ro; }
      } break;
      case 'textcleartags': {
        if (el && el.__iupRender) { el.__iupTags = []; el.__iupRender(); }
      } break;
      case 'textplaceholder': {
        if (el) el.placeholder = c.value;
      } break;
      case 'textmaxlen': {
        if (el) { if (c.n > 0) el.maxLength = c.n; else el.removeAttribute('maxlength'); }
      } break;
      case 'textsetcaret': {
        if (el && el.setSelectionRange) el.setSelectionRange(c.pos, c.pos);
      } break;
      case 'textsetselpos': {
        if (el && el.setSelectionRange) el.setSelectionRange(c.start, c.end);
      } break;
      case 'textinsert': {
        if (el) {
          var ins = el.selectionStart || 0, ine = el.selectionEnd || 0;
          el.value = el.value.slice(0, ins) + c.value + el.value.slice(ine);
          el.setSelectionRange(ins + c.value.length, ins + c.value.length);
        }
      } break;
      case 'texttabsize': {
        if (el) el.style.tabSize = c.n;
      } break;
      case 'textsetcaretlc': {
        if (el && el.setSelectionRange) { var clp = iupLcToPos(el.value, c.lin, c.col); el.setSelectionRange(clp, clp); }
      } break;
      case 'textsetsellc': {
        if (el && el.setSelectionRange) { var ss = iupLcToPos(el.value, c.l1, c.c1), se = iupLcToPos(el.value, c.l2, c.c2); el.setSelectionRange(ss, se); }
      } break;
      case 'textclipboard': {
        if (el) {
          el.focus();
          var act = c.action.toUpperCase();
          if (act === 'CLEAR') { var cs = el.selectionStart || 0, ce = el.selectionEnd || 0; el.value = el.value.slice(0, cs) + el.value.slice(ce); el.setSelectionRange(cs, cs); }
          else { try { document.execCommand(act === 'CUT' ? 'cut' : act === 'PASTE' ? 'paste' : 'copy'); } catch (e) {} }
        }
      } break;
      case 'textscrolltopos': {
        if (el) {
          var lh = parseFloat(getComputedStyle(el).lineHeight) || 16;
          var ln = el.value.slice(0, c.pos).split('\n').length - 1;
          el.scrollTop = ln * lh;
        }
      } break;
      case 'textpassword': {
        if (el && el.type !== undefined) el.type = c.on ? 'password' : 'text';
      } break;
      case 'textwire': {
        if (el) {
          var wid = c.id;
          el.addEventListener('beforeinput', function (e) {
            var v = el.value, s = el.selectionStart || 0, en = el.selectionEnd || 0, ch = 0, nv = v;
            // OVERWRITE: a collapsed caret not at end replaces the next char
            if (el.__iupOverwrite && e.inputType === 'insertText' && s === en && s < v.length) { en = s + 1; el.setSelectionRange(s, en); }
            if (e.inputType === 'insertText' && e.data) { ch = e.data.charCodeAt(0); nv = v.slice(0, s) + e.data + v.slice(en); }
            else if (e.inputType === 'deleteContentBackward') { nv = v.slice(0, (s === en) ? Math.max(0, s - 1) : s) + v.slice(en); }
            if (dispatch.sync) {
              var ret = Dr('iupwasmDispatchTextAction', 'number', ['number', 'number', 'string'], [wid, ch, nv]);
              if (ret === -1) e.preventDefault();
              else if (ret > 0 && ret !== ch) {
                e.preventDefault();
                el.value = v.slice(0, s) + String.fromCharCode(ret) + v.slice(en);
                el.selectionStart = el.selectionEnd = s + 1;
                D('iupwasmDispatchValueChanged', wid);
              }
            } else {
              Dt('iupwasmDispatchTextAction', ['number', 'number', 'string'], [wid, ch, nv]);
            }
          });
          el.addEventListener('input', function () {
            var m = el.__iupFilter;
            if (m) {
              var v = el.value, nv = m === 'NUMBER' ? v.replace(/[^0-9+\-.eE]/g, '') : m === 'UPPERCASE' ? v.toUpperCase() : m === 'LOWERCASE' ? v.toLowerCase() : v;
              if (nv !== v) { var p = el.selectionStart; el.value = nv; try { el.setSelectionRange(p, p); } catch (_) {} }
            }
            D('iupwasmDispatchValueChanged', wid);
          });
          var reportCaret = function () {
            var p = el.selectionStart || 0, lc = iupPosToLc(el.value, p).split(',');
            Dt('iupwasmDispatchTextCaret', ['number', 'number', 'number', 'number'], [wid, +lc[0], +lc[1], p]);
          };
          el.addEventListener('keyup', reportCaret);
          el.addEventListener('click', reportCaret);
          el.addEventListener('select', reportCaret);
        }
      } break;
      case 'textfilter': {
        if (el) el.__iupFilter = c.mode ? c.mode.toUpperCase() : '';
      } break;
      case 'textoverwrite': {
        if (el) el.__iupOverwrite = !!c.on;
      } break;
      case 'textscrollto': {
        if (el) {
          var tsl = parseFloat(getComputedStyle(el).lineHeight) || 16;
          el.scrollTop = (c.lin - 1) * tsl;
        }
      } break;
      case 'createvlist': {
        var vw = document.createElement('div');
        vw.style.position = 'absolute'; vw.style.boxSizing = 'border-box'; vw.style.overflow = 'auto';
        vw.style.border = '1px solid var(--iup-bd)'; vw.style.background = 'var(--iup-txtbg)'; vw.style.height = '100px';
        var vinner = document.createElement('div'); vinner.style.position = 'relative';
        vw.appendChild(vinner);
        vw.__iupVInner = vinner; vw.__iupVRows = []; vw.__iupVSel = -1; vw.__iupFitImage = true;
        els[c.id] = vw; vw.dataset.iupId = c.id;
      } break;
      case 'vlistinit': {
        if (el) {
          var vid = c.id; el.__iupItemH = c.itemH;
          var vpending = false;
          el.addEventListener('scroll', function () { if (vpending) return; vpending = true; requestAnimationFrame(function () { vpending = false; D('iupwasmListVScroll', vid); }); });
          if (typeof ResizeObserver !== 'undefined') new ResizeObserver(function () { D('iupwasmListVScroll', vid); }).observe(el);
        }
      } break;
      case 'vlistcount': {
        if (el) { el.__iupCount = c.count; el.__iupVInner.style.height = (c.count * el.__iupItemH) + 'px'; }
      } break;
      case 'vlistfit': {
        if (el) { el.__iupFitImage = !!c.fit; var vfr = el.__iupVRows; for (var i = 0; i < vfr.length; i++) vfr[i].__iupImg.style.height = c.fit ? ((el.__iupItemH - 4) + 'px') : ''; }
      } break;
      case 'vlistwindow': {
        if (el) {
          var vwid = c.id, vinner2 = el.__iupVInner, vrows = el.__iupVRows, vih = el.__iupItemH;
          while (vrows.length < c.count) {
            var vd = document.createElement('div');
            vd.style.position = 'absolute'; vd.style.left = '0'; vd.style.right = '0'; vd.style.height = vih + 'px';
            vd.style.display = 'flex'; vd.style.alignItems = 'center'; vd.style.padding = '0 4px'; vd.style.boxSizing = 'border-box';
            vd.style.cursor = 'pointer'; vd.style.whiteSpace = 'nowrap'; vd.style.overflow = 'hidden';
            var vimg = document.createElement('img'); vimg.style.marginRight = '4px'; vimg.style.display = 'none';
            if (el.__iupFitImage) vimg.style.height = (vih - 4) + 'px';
            var vsp = document.createElement('span');
            vd.appendChild(vimg); vd.appendChild(vsp); vd.__iupImg = vimg; vd.__iupSpan = vsp;
            vd.addEventListener('click', function () { D('iupwasmListVClick', vwid, this.__iupPos); });
            vinner2.appendChild(vd); vrows.push(vd);
          }
          while (vrows.length > c.count) vinner2.removeChild(vrows.pop());
        }
      } break;
      case 'vlistcell': {
        if (el) {
          var vcd = el.__iupVRows[c.rowIdx];
          if (vcd) {
            vcd.__iupPos = c.pos; vcd.style.top = ((c.pos - 1) * el.__iupItemH) + 'px'; vcd.__iupSpan.textContent = c.text;
            var vcim = c.imgId ? (globalThis.__iupImg && globalThis.__iupImg.map[c.imgId]) : null;
            if (vcim) { vcd.__iupImg.src = vcim.url; vcd.__iupImg.style.display = ''; } else vcd.__iupImg.style.display = 'none';
            vcd.style.background = (c.pos === el.__iupVSel) ? 'var(--iup-accent-bg)' : '';
          }
        }
      } break;
      case 'vlistselect': {
        if (el) { el.__iupVSel = c.pos; var vsr = el.__iupVRows; for (var i = 0; i < vsr.length; i++) vsr[i].style.background = (vsr[i].__iupPos === c.pos) ? 'var(--iup-accent-bg)' : ''; }
      } break;
      case 'createlist': {
        var lwrap = document.createElement('div');
        lwrap.style.position = 'absolute'; lwrap.style.boxSizing = 'border-box';
        lwrap.style.display = 'flex'; lwrap.style.flexDirection = 'column';
        var lval, lopts;
        if (c.dropdown && c.editbox) {
          var linput = document.createElement('input');
          var ldl = document.createElement('datalist');
          lwrap.style.flexDirection = 'row'; lwrap.style.border = '1px solid var(--iup-bd)'; lwrap.style.background = 'var(--iup-txtbg)';
          linput.style.flex = '1'; linput.style.minWidth = '0'; linput.style.border = 'none'; linput.style.outline = 'none'; linput.style.background = 'transparent';
          var lbtn = document.createElement('div');
          lbtn.style.cssText = 'width:20px;flex:0 0 20px;display:flex;align-items:center;justify-content:center;cursor:default;';
          var lchev = document.createElement('span');  // CSS chevron, to match the native <select> arrow
          lchev.style.cssText = 'display:block;width:7px;height:7px;margin-top:-3px;border-right:1.5px solid var(--iup-bd-dark);border-bottom:1.5px solid var(--iup-bd-dark);transform:rotate(45deg);';
          lbtn.appendChild(lchev);
          var lpop = document.createElement('div');
          lpop.style.cssText = 'position:fixed;display:none;background:var(--iup-txtbg);border:1px solid var(--iup-bd);max-height:160px;overflow:auto;z-index:3000;box-shadow:0 2px 6px rgba(0,0,0,0.2);';
          document.body.appendChild(lpop);
          lwrap.__iupOpenPop = function (show) {
            if (!show) { lpop.style.display = 'none'; return; }
            lpop.innerHTML = '';
            for (var i = 0; i < ldl.options.length; i++) {
              var item = document.createElement('div');
              item.textContent = ldl.options[i].value;
              item.style.cssText = 'padding:2px 8px;cursor:default;white-space:nowrap;';
              item.addEventListener('mouseenter', function () { this.style.background = 'var(--iup-accent-bg)'; });
              item.addEventListener('mouseleave', function () { this.style.background = ''; });
              item.addEventListener('mousedown', function (ev) {
                linput.value = this.textContent; lpop.style.display = 'none';
                linput.dispatchEvent(new Event('input', { bubbles: true })); ev.preventDefault();
              });
              lpop.appendChild(item);
            }
            var lr = lwrap.getBoundingClientRect();
            lpop.style.left = lr.left + 'px'; lpop.style.top = lr.bottom + 'px';
            // DROPEXPAND=NO pins the popup to the control width; YES lets it grow to content
            if (lwrap.__iupDropExpand === false) { lpop.style.width = lr.width + 'px'; lpop.style.minWidth = ''; }
            else { lpop.style.minWidth = lr.width + 'px'; lpop.style.width = ''; }
            lpop.style.display = 'block';
          };
          lbtn.addEventListener('mousedown', function (e) {
            e.preventDefault();
            lwrap.__iupOpenPop(lpop.style.display !== 'block');
          });
          document.addEventListener('mousedown', function (e) { if (!lbtn.contains(e.target) && !lpop.contains(e.target)) lpop.style.display = 'none'; });
          lwrap.appendChild(linput); lwrap.appendChild(ldl); lwrap.appendChild(lbtn);
          lwrap.__iupPop = lpop; lval = linput; lopts = ldl;
        } else if (c.editbox) {
          var linput2 = document.createElement('input');
          var lsel2 = document.createElement('select');
          lsel2.size = c.size > 1 ? c.size : 5;
          linput2.style.width = '100%'; linput2.style.boxSizing = 'border-box';
          lsel2.style.flex = '1'; lsel2.style.width = '100%'; lsel2.style.boxSizing = 'border-box';
          lwrap.appendChild(linput2); lwrap.appendChild(lsel2);
          lval = linput2; lopts = lsel2;
        } else {
          var lsel = document.createElement('select');
          if (!c.dropdown) lsel.size = c.size > 1 ? c.size : 5;
          if (c.multiple) lsel.multiple = true;
          lsel.style.flex = '1'; lsel.style.width = '100%'; lsel.style.boxSizing = 'border-box';
          lwrap.appendChild(lsel);
          lval = lsel; lopts = lsel;
        }
        lwrap.__iupVal = lval; lwrap.__iupOpts = lopts;
        els[c.id] = lwrap; lwrap.dataset.iupId = c.id;
      } break;
      case 'listappend': {
        if (el) { var lo = document.createElement('option'); lo.text = c.text; if (el.__iupSpacing) { lo.style.paddingTop = el.__iupSpacing + 'px'; lo.style.paddingBottom = el.__iupSpacing + 'px'; } el.__iupOpts.appendChild(lo); }
      } break;
      case 'listinsert': {
        if (el) { var lo2 = document.createElement('option'); lo2.text = c.text; if (el.__iupSpacing) { lo2.style.paddingTop = el.__iupSpacing + 'px'; lo2.style.paddingBottom = el.__iupSpacing + 'px'; } var lop = el.__iupOpts; if (c.pos >= 0 && c.pos < lop.children.length) lop.insertBefore(lo2, lop.children[c.pos]); else lop.appendChild(lo2); }
      } break;
      case 'listitemimage': {
        if (el && el.__iupOpts) {
          var lopt = el.__iupOpts.children[c.pos];
          if (lopt) {
            var liim = c.imgId ? (globalThis.__iupImg && globalThis.__iupImg.map[c.imgId]) : null;
            if (liim) {
              var liw = c.w > 0 ? c.w : liim.w, lih = c.h > 0 ? c.h : liim.h;
              lopt.style.backgroundImage = 'url(' + liim.url + ')';
              lopt.style.backgroundRepeat = 'no-repeat'; lopt.style.backgroundPosition = '2px center';
              lopt.style.backgroundSize = liw + 'px ' + lih + 'px';
              lopt.style.paddingLeft = (liw + 6) + 'px'; lopt.__iupImgId = c.imgId;
            } else { lopt.style.backgroundImage = ''; lopt.style.paddingLeft = ''; lopt.__iupImgId = 0; }
          }
        }
      } break;
      case 'listtopitem': {
        if (el && el.__iupOpts) { var lti = el.__iupOpts.children[c.pos]; if (lti) el.__iupOpts.scrollTop = lti.offsetTop; }
      } break;
      case 'vlistsettop': {
        if (el) el.scrollTop = c.top;
      } break;
      case 'listspacing': {
        if (el && el.__iupOpts) {
          el.__iupSpacing = c.s; var lspo = el.__iupOpts.children;
          for (var i = 0; i < lspo.length; i++) {
            lspo[i].style.paddingTop = c.s + 'px'; lspo[i].style.paddingBottom = c.s + 'px';
            if (!lspo[i].__iupImgId) { lspo[i].style.paddingLeft = c.s + 'px'; lspo[i].style.paddingRight = c.s + 'px'; }
          }
        }
      } break;
      case 'listpadding': {
        if (el && el.__iupVal) el.__iupVal.style.padding = c.v + 'px ' + c.h + 'px';
      } break;
      case 'listshowdropdown': {
        if (el && el.__iupOpenPop) el.__iupOpenPop(!!c.on);
      } break;
      case 'listdropexpand': {
        if (el) el.__iupDropExpand = !!c.on;
      } break;
      case 'listdragdrop':
      case 'listdndsrc':
      case 'listdndtgt': {
        var lds = el && el.__iupOpts;
        if (lds && lds.tagName === 'SELECT') {
          var LM = globalThis.__iupListDnd;
          if (!LM) {
            LM = globalThis.__iupListDnd = { src: null };
            LM.hit = function (sel, y) { for (var i = 0; i < sel.options.length; i++) if (y <= sel.options[i].getBoundingClientRect().bottom) return i; return -1; };
            window.addEventListener('mousemove', function (e) { if (LM.src && (e.buttons & 1)) LM.src.moved = true; });
            window.addEventListener('mouseup', function (e) {
              var s = LM.src; LM.src = null;
              if (!s || !s.moved) return;
              var tn = document.elementFromPoint(e.clientX, e.clientY);
              var tsel = tn && tn.closest ? tn.closest('select') : null;
              if (!tsel || tsel.__iupListId == null) return;
              if (tsel === s.sel) { if (s.reorder) { var to = LM.hit(tsel, e.clientY); if (to !== s.from) D('iupwasmListReorder', s.id, s.from, to); } }
              else if (tsel.__iupLDrop && s.dnd) { var tr = tsel.getBoundingClientRect(); D('iupwasmDndTransfer', s.id, tsel.__iupListId, s.fromY, Math.round(e.clientY - tr.top)); }
            });
          }
          lds.__iupListId = c.id;
          if (c.op === 'listdragdrop') lds.__iupLReorder = 1;
          if (c.op === 'listdndsrc') lds.__iupLDnd = !!c.on;
          if (c.op === 'listdndtgt') lds.__iupLDrop = !!c.on;
          if ((c.op === 'listdragdrop' || c.op === 'listdndsrc') && !lds.__iupLSrcWired) {
            lds.__iupLSrcWired = 1; var lsid = c.id;
            lds.addEventListener('mousedown', function (e) {
              var r = lds.getBoundingClientRect();
              LM.src = { sel: lds, id: lsid, from: LM.hit(lds, e.clientY), fromY: Math.round(e.clientY - r.top), moved: false, reorder: !!lds.__iupLReorder, dnd: !!lds.__iupLDnd };
            });
          }
        }
      } break;
      case 'listremove': {
        if (el) { var lop2 = el.__iupOpts; if (c.pos >= 0 && c.pos < lop2.children.length) lop2.removeChild(lop2.children[c.pos]); }
      } break;
      case 'listremoveall': {
        if (el) { var lop3 = el.__iupOpts; while (lop3.firstChild) lop3.removeChild(lop3.firstChild); }
      } break;
      case 'listselindex': {
        if (el && el.__iupOpts.selectedIndex !== undefined) el.__iupOpts.selectedIndex = c.idx;
      } break;
      case 'listsetmulti': {
        if (el) { var ls = c.str, lmo = el.__iupOpts.options; for (var i = 0; i < lmo.length; i++) lmo[i].selected = (ls.charAt(i) === '+'); el.__iupOpts.__iupPrev = Array.prototype.map.call(lmo, function (o) { return o.selected ? 1 : 0; }); }
      } break;
      case 'listsettext': {
        if (el) el.__iupVal.value = c.str;
      } break;
      case 'listeditreadonly': {
        if (el && el.__iupVal) el.__iupVal.readOnly = !!c.ro;
      } break;
      case 'listeditmaxlen': {
        if (el && el.__iupVal) { if (c.n > 0) el.__iupVal.maxLength = c.n; else el.__iupVal.removeAttribute('maxlength'); }
      } break;
      case 'listeditappend': {
        if (el && el.__iupVal) el.__iupVal.value += c.value;
      } break;
      case 'listeditinsert': {
        if (el && el.__iupVal) { var liv = el.__iupVal, lis = liv.selectionStart || 0, lie = liv.selectionEnd || 0; liv.value = liv.value.slice(0, lis) + c.value + liv.value.slice(lie); liv.setSelectionRange(lis + c.value.length, lis + c.value.length); }
      } break;
      case 'listeditclipboard': {
        if (el && el.__iupVal) {
          var lcv = el.__iupVal; lcv.focus(); var la = c.action.toUpperCase();
          if (la === 'CLEAR') { var lcs = lcv.selectionStart || 0, lce = lcv.selectionEnd || 0; lcv.value = lcv.value.slice(0, lcs) + lcv.value.slice(lce); lcv.setSelectionRange(lcs, lcs); }
          else { try { document.execCommand(la === 'CUT' ? 'cut' : la === 'PASTE' ? 'paste' : 'copy'); } catch (e) {} }
        }
      } break;
      case 'listeditcaret': {
        if (el && el.__iupVal && el.__iupVal.setSelectionRange) el.__iupVal.setSelectionRange(c.pos, c.pos);
      } break;
      case 'listeditselpos': {
        if (el && el.__iupVal && el.__iupVal.setSelectionRange) el.__iupVal.setSelectionRange(c.s, c.e);
      } break;
      case 'listwire': {
        if (el) {
          var lid = c.id, lwval = el.__iupVal, lwopts = el.__iupOpts;
          if (c.dropdown) lwval.addEventListener('mousedown', function () { D('iupwasmDispatchListDropdown', lid, 1); });
          if (c.editbox) {
            lwval.addEventListener('input', function () {
              D('iupwasmDispatchValueChanged', lid);
              Dt('iupwasmDispatchListEdit', ['number', 'number', 'string'], [lid, 0, lwval.value]);
            });
            var reportListCaret = function () { Dt('iupwasmDispatchListCaret', ['number', 'number'], [lid, lwval.selectionStart || 0]); };
            lwval.addEventListener('keyup', reportListCaret);
            lwval.addEventListener('click', reportListCaret);
            lwval.addEventListener('select', reportListCaret);
          }
          if (lwopts.tagName === 'SELECT') {
            lwopts.addEventListener('change', function () {
              if (c.multiple) {
                var sel = '';
                for (var i = 0; i < lwopts.options.length; i++) sel += lwopts.options[i].selected ? '+' : '-';
                Dt('iupwasmDispatchListMulti', ['number', 'string'], [lid, sel]);
              } else {
                var lidx = lwopts.selectedIndex;
                if (lidx >= 0) Dt('iupwasmDispatchListAction', ['number', 'number', 'number', 'string'], [lid, lidx + 1, 1, lwopts.options[lidx].text]);
              }
              D('iupwasmDispatchValueChanged', lid);
            });
            lwopts.addEventListener('dblclick', function () {
              var lidx2 = lwopts.selectedIndex;
              if (lidx2 >= 0) Dt('iupwasmDispatchListDblclick', ['number', 'number', 'string'], [lid, lidx2 + 1, lwopts.options[lidx2].text]);
            });
          }
        }
      } break;
      case 'tablecreate': {
        var twr = document.createElement('div');
        twr.style.position = 'absolute'; twr.style.boxSizing = 'border-box'; twr.style.overflow = 'auto';
        twr.style.border = '1px solid var(--iup-bd)'; twr.style.background = 'var(--iup-txtbg)';
        var ttable = document.createElement('table'); ttable.style.borderCollapse = 'collapse'; ttable.style.width = '100%';
        var tthead = document.createElement('thead'); var thr = document.createElement('tr'); tthead.appendChild(thr);
        var ttbody = document.createElement('tbody'); ttable.appendChild(tthead); ttable.appendChild(ttbody); twr.appendChild(ttable);
        twr.__iupHead = thr; twr.__iupBody = ttbody; twr.__iupGrid = 1;
        els[c.id] = twr; twr.dataset.iupId = c.id;
        var tcid = c.id;
        ttable.addEventListener('click', function (e) {
          var th = e.target.closest('th'); if (th && th.dataset.col) { D('iupwasmTableHeaderClick', tcid, +th.dataset.col); return; }
          var td = e.target.closest('td'); if (!td || td.dataset.lin === undefined) return;
          D('iupwasmTableCellClick', tcid, +td.dataset.lin, +td.dataset.col);
        });
        ttable.addEventListener('dblclick', function (e) {
          var td = e.target.closest('td'); if (!td || td.__iupEditing) return;
          var lin = +td.dataset.lin, col = +td.dataset.col;
          if (!Dr('iupwasmTableEditBegin', 'number', ['number', 'number', 'number'], [tcid, lin, col])) return;
          var inp = document.createElement('input'); inp.value = td.__iupText || ''; inp.style.width = '100%'; inp.style.boxSizing = 'border-box'; inp.style.font = 'inherit';
          td.__iupEditing = true; td.textContent = ''; td.appendChild(inp); inp.focus(); inp.select();
          function commit() {
            if (!td.__iupEditing) return; td.__iupEditing = false;
            var applied = Dr('iupwasmTableEditEnd', 'number', ['number', 'number', 'number', 'string', 'number'], [tcid, lin, col, inp.value, 1]);
            if (!applied) apply({ op: 'tablerender', id: tcid, lin: lin, col: col });  // rejected: restore original
          }
          inp.addEventListener('blur', commit);
          inp.addEventListener('keydown', function (ev) {
            if (ev.key === 'Enter') { ev.preventDefault(); inp.blur(); }
            else if (ev.key === 'Escape') { td.__iupEditing = false; Dr('iupwasmTableEditEnd', 'number', ['number', 'number', 'number', 'string', 'number'], [tcid, lin, col, inp.value, 0]); apply({ op: 'tablerender', id: tcid, lin: lin, col: col }); }
          });
        });
      } break;
      case 'tablefeatures': {
        if (el) { el.__iupReorder = !!c.reorder; el.__iupResize = !!c.resize; el.__iupRowDnd = !!c.dragdrop; }
      } break;
      case 'tablegrid': {
        if (el) { el.__iupGrid = c.show; var tgc = el.querySelectorAll('td, th'); for (var i = 0; i < tgc.length; i++) tgc[i].style.border = c.show ? '1px solid var(--iup-bd-faint)' : 'none'; }
      } break;
      case 'tablebuild': {
        if (el) {
          var tbh = el.__iupHead, tbb = el.__iupBody, tbg = el.__iupGrid;
          var tbstyle = function (cell, header) {
            cell.style.padding = '2px 6px'; cell.style.border = tbg ? '1px solid var(--iup-bd-faint)' : 'none'; cell.style.whiteSpace = 'nowrap';
            if (header) { cell.style.background = 'var(--iup-bg)'; cell.style.textAlign = 'left'; cell.style.position = 'sticky'; cell.style.top = '0'; }
          };
          while (tbh.children.length > c.numcol) tbh.removeChild(tbh.lastChild);
          while (tbh.children.length < c.numcol) {
            var tbth = document.createElement('th'); tbstyle(tbth, true); tbth.dataset.col = tbh.children.length + 1; tbh.appendChild(tbth);
            var tbts = document.createElement('span'); tbth.appendChild(tbts); tbth.__iupTitleSpan = tbts;
            if (el.__iupReorder) {
              tbth.draggable = true;
              tbth.addEventListener('dragstart', function (e) { el.__iupDragCol = +this.dataset.col; if (e.dataTransfer) e.dataTransfer.effectAllowed = 'move'; });
              tbth.addEventListener('dragover', function (e) { e.preventDefault(); });
              tbth.addEventListener('drop', function (e) {
                e.preventDefault(); var from = el.__iupDragCol, to = +this.dataset.col;
                if (from && to && from !== to && Dr('iupwasmTableReorder', 'number', ['number', 'number', 'number'], [c.id, from, to])) tableReorderCols(el, from, to);
              });
            }
            if (el.__iupResize) {
              var trh = document.createElement('div');
              trh.style.cssText = 'position:absolute;right:0;top:0;width:5px;height:100%;cursor:col-resize;user-select:none;';
              trh.draggable = false;
              (function (th2) {
                trh.addEventListener('mousedown', function (e) {
                  e.preventDefault(); e.stopPropagation();
                  var sx = e.clientX, sw = th2.offsetWidth;
                  var mv = function (ev) { th2.style.width = Math.max(20, sw + (ev.clientX - sx)) + 'px'; };
                  var up = function () { document.removeEventListener('mousemove', mv); document.removeEventListener('mouseup', up); };
                  document.addEventListener('mousemove', mv); document.addEventListener('mouseup', up);
                });
              })(tbth);
              tbth.appendChild(trh);
            }
          }
          while (tbb.children.length > c.numlin) tbb.removeChild(tbb.lastChild);
          for (var l = 0; l < c.numlin; l++) {
            var tbtr = tbb.children[l];
            if (!tbtr) {
              tbtr = document.createElement('tr'); tbb.appendChild(tbtr);
              if (el.__iupRowDnd) {
                tbtr.draggable = true;
                tbtr.addEventListener('dragstart', function (e) { el.__iupDragRow = tableRowLineOf(el, this); if (e.dataTransfer) e.dataTransfer.effectAllowed = 'move'; });
                tbtr.addEventListener('dragover', function (e) {
                  if (!el.__iupDragRow) return;
                  e.preventDefault();
                  var r = this.getBoundingClientRect();
                  var above = e.clientY < r.top + r.height / 2;
                  tableClearRowDrop(el);
                  this.style.boxShadow = above ? 'inset 0 2px 0 0 #3584e4' : 'inset 0 -2px 0 0 #3584e4';
                  el.__iupDropRow = this;
                });
                tbtr.addEventListener('drop', function (e) {
                  e.preventDefault();
                  var from = el.__iupDragRow; el.__iupDragRow = 0;
                  var r = this.getBoundingClientRect();
                  var line = tableRowLineOf(el, this);
                  var before = (e.clientY < r.top + r.height / 2) ? line : line + 1;
                  tableClearRowDrop(el);
                  if (from) Dr('iupwasmTableRowDragDrop', 'number', ['number', 'number', 'number'], [c.id, from, before]);
                });
                tbtr.addEventListener('dragend', function () { el.__iupDragRow = 0; tableClearRowDrop(el); });
              }
            }
            while (tbtr.children.length > c.numcol) tbtr.removeChild(tbtr.lastChild);
            while (tbtr.children.length < c.numcol) { var tbtd = document.createElement('td'); tbstyle(tbtd, false); tbtd.dataset.lin = l + 1; tbtd.dataset.col = tbtr.children.length + 1; tbtd.__iupText = ''; tbtr.appendChild(tbtd); }
          }
        }
      } break;
      case 'tablemoverow': {
        if (el) {
          var tmb = el.__iupBody, tmf = c.from - 1, tmt = c.to - 1;
          var tmrows = tmb.children;
          if (tmf >= 0 && tmf < tmrows.length && tmt >= 0 && tmt < tmrows.length && tmf !== tmt) {
            var tmrow = tmrows[tmf];
            tmb.removeChild(tmrow);
            var tmref = tmb.children[tmt];
            if (tmref) tmb.insertBefore(tmrow, tmref); else tmb.appendChild(tmrow);
            for (var tmi = 0; tmi < tmb.children.length; tmi++) {
              var tmr = tmb.children[tmi];
              for (var tmj = 0; tmj < tmr.children.length; tmj++) tmr.children[tmj].dataset.lin = tmi + 1;
            }
          }
        }
      } break;
      case 'tablecoltitle': {
        if (el) { var tcth = el.__iupHead.children[c.col - 1]; if (tcth) { if (tcth.__iupTitleSpan) tcth.__iupTitleSpan.textContent = c.title; else tcth.textContent = c.title; } }
      } break;
      case 'tablecolwidth': {
        if (el) { var tcwh = el.__iupHead.children[c.col - 1]; if (tcwh) tcwh.style.width = c.w + 'px'; }
      } break;
      case 'tablerender': {
        if (el) {
          var trr = el.__iupBody.children[c.lin - 1];
          if (trr) {
            var trtd = trr.children[c.col - 1];
            if (trtd) {
              trtd.__iupEditing = false; trtd.textContent = '';
              if (trtd.__iupImg) {
                var trim = globalThis.__iupImg && globalThis.__iupImg.map[trtd.__iupImg];
                if (trim) { var trimg = document.createElement('img'); trimg.src = trim.url; trimg.style.verticalAlign = 'middle'; trimg.style.marginRight = '4px'; if (el.__iupFitImage && el.__iupImgMaxH) { trimg.style.maxHeight = el.__iupImgMaxH + 'px'; trimg.style.width = 'auto'; } trtd.appendChild(trimg); }
              }
              trtd.appendChild(document.createTextNode(trtd.__iupText || ''));
            }
          }
        }
      } break;
      case 'tablesetcell': {
        if (el) { var tscr = el.__iupBody.children[c.lin - 1]; if (tscr) { var tsctd = tscr.children[c.col - 1]; if (tsctd) { tsctd.__iupText = c.value; apply({ op: 'tablerender', id: c.id, lin: c.lin, col: c.col }); } } }
      } break;
      case 'tablefitimage': {
        if (el) {
          el.__iupFitImage = !!c.fit; el.__iupImgMaxH = c.maxH;
          var tfrows = el.__iupBody ? el.__iupBody.children : [];
          for (var r = 0; r < tfrows.length; r++) { var tfimgs = tfrows[r].querySelectorAll('img'); for (var i = 0; i < tfimgs.length; i++) { if (c.fit && c.maxH) { tfimgs[i].style.maxHeight = c.maxH + 'px'; tfimgs[i].style.width = 'auto'; } else { tfimgs[i].style.maxHeight = ''; tfimgs[i].style.width = ''; } } }
        }
      } break;
      case 'tablecellimage': {
        if (el) { var tcir = el.__iupBody.children[c.lin - 1]; if (tcir) { var tcitd = tcir.children[c.col - 1]; if (tcitd) { tcitd.__iupImg = c.imgId; apply({ op: 'tablerender', id: c.id, lin: c.lin, col: c.col }); } } }
      } break;
      case 'tablestripe': {
        if (el) {
          var tsrows = el.__iupBody.children;
          for (var i = 0; i < tsrows.length; i++) { var tsbg = c.alt ? ((i % 2) ? c.odd : c.even) : ''; tsrows[i].__iupStripeBg = tsbg; if (tsrows[i] !== el.__iupSelRow) tsrows[i].style.background = tsbg; }
          el.__iupEven = c.even; el.__iupOdd = c.odd; el.__iupAlt = c.alt;
        }
      } break;
      case 'tablecolalign': {
        if (el) {
          if (!el.__iupAligns) el.__iupAligns = {};
          el.__iupAligns[c.col] = c.css;
          var tath = el.__iupHead.children[c.col - 1]; if (tath) tath.style.textAlign = c.css;
          var tarows = el.__iupBody.children;
          for (var i = 0; i < tarows.length; i++) { var tatd = tarows[i].children[c.col - 1]; if (tatd && tatd.dataset.lin !== undefined) tatd.style.textAlign = c.css; }
        }
      } break;
      case 'tablecellcolor': {
        if (el) { var tccr = el.__iupBody.children[c.lin - 1]; if (tccr) { var tcctd = tccr.children[c.col - 1]; if (tcctd) { tcctd.style.background = c.bg; tcctd.style.color = c.fg; } } }
      } break;
      case 'tablevirtualinit': {
        if (el) {
          var tvid = c.id, tvbody = el.__iupBody;
          var tvspacer = function () { var tr = document.createElement('tr'); var td = document.createElement('td'); td.style.padding = '0'; td.style.border = '0'; tr.appendChild(td); return tr; };
          el.__iupTopSpacer = tvspacer(); el.__iupBotSpacer = tvspacer();
          tvbody.appendChild(el.__iupTopSpacer); tvbody.appendChild(el.__iupBotSpacer);
          el.__iupVRows = []; el.__iupVirtual = true;
          var tvpending = false;
          el.addEventListener('scroll', function () { if (tvpending) return; tvpending = true; requestAnimationFrame(function () { tvpending = false; D('iupwasmTableVScroll', tvid); }); });
          if (typeof ResizeObserver !== 'undefined') new ResizeObserver(function () { D('iupwasmTableVScroll', tvid); }).observe(el);
        }
      } break;
      case 'tablevwindow': {
        if (el && el.__iupVirtual) {
          var tvwbody = el.__iupBody, tvwrows = el.__iupVRows, tvwgrid = el.__iupGrid;
          el.__iupTopSpacer.firstChild.style.height = c.top + 'px'; el.__iupTopSpacer.firstChild.colSpan = c.numCol;
          el.__iupBotSpacer.firstChild.style.height = c.bot + 'px'; el.__iupBotSpacer.firstChild.colSpan = c.numCol;
          var tvmk = function () { var td = document.createElement('td'); td.style.padding = '2px 6px'; td.style.whiteSpace = 'nowrap'; td.style.border = tvwgrid ? '1px solid var(--iup-bd-faint)' : 'none'; return td; };
          while (tvwrows.length < c.count) { var tvtr = document.createElement('tr'); for (var cc = 0; cc < c.numCol; cc++) tvtr.appendChild(tvmk()); tvwbody.insertBefore(tvtr, el.__iupBotSpacer); tvwrows.push(tvtr); }
          while (tvwrows.length > c.count) tvwbody.removeChild(tvwrows.pop());
          for (var i = 0; i < tvwrows.length; i++) { var tvr = tvwrows[i]; while (tvr.children.length > c.numCol) tvr.removeChild(tvr.lastChild); while (tvr.children.length < c.numCol) tvr.appendChild(tvmk()); }
        }
      } break;
      case 'tablevcell': {
        if (el) { var tvctr = el.__iupVRows[c.rowIdx]; if (tvctr) { var tvctd = tvctr.children[c.col]; if (tvctd) {
          tvctd.textContent = '';
          var tvcim = c.imgId ? (globalThis.__iupImg && globalThis.__iupImg.map[c.imgId]) : null;
          if (tvcim) { var tvcimg = document.createElement('img'); tvcimg.src = tvcim.url; tvcimg.style.verticalAlign = 'middle'; tvcimg.style.marginRight = '4px'; if (el.__iupFitImage && el.__iupImgMaxH) { tvcimg.style.maxHeight = el.__iupImgMaxH + 'px'; tvcimg.style.width = 'auto'; } tvctd.appendChild(tvcimg); }
          tvctd.appendChild(document.createTextNode(c.str || ''));
          tvctd.dataset.lin = c.lin; tvctd.dataset.col = c.col + 1; if (el.__iupAligns && el.__iupAligns[c.col + 1]) tvctd.style.textAlign = el.__iupAligns[c.col + 1]; } } }
      } break;
      case 'tablevstripe': {
        if (el) { var tvstr = el.__iupVRows[c.rowIdx]; if (tvstr) tvstr.style.background = el.__iupAlt ? (((c.lin - 1) % 2) ? el.__iupOdd : el.__iupEven) : ''; }
      } break;
      case 'tablefocus': {
        if (el) {
          var tfprev = el.__iupFocusTd; if (tfprev) tfprev.style.outline = '';
          var tftr = el.__iupBody.children[c.lin - 1];
          var tftd = tftr ? tftr.children[c.col - 1] : null;
          var tfprevRow = el.__iupSelRow;
          if (tfprevRow && tfprevRow !== tftr) { tfprevRow.style.background = tfprevRow.__iupStripeBg || ''; tfprevRow.style.color = ''; }
          if (tftr && c.select) { tftr.style.background = 'Highlight'; tftr.style.color = 'HighlightText'; el.__iupSelRow = tftr; } else { el.__iupSelRow = null; }
          if (tftd) { if (c.focusrect) { tftd.style.outline = '1px dotted ' + (c.select ? 'HighlightText' : 'var(--iup-accent)'); tftd.style.outlineOffset = '-1px'; } el.__iupFocusTd = c.focusrect ? tftd : null; tftd.scrollIntoView({ block: 'nearest', inline: 'nearest' }); }
        }
      } break;
      case 'tablescrollto': {
        if (el) { var tstr2 = el.__iupBody.children[c.lin - 1]; var tstd2 = tstr2 ? tstr2.children[c.col - 1] : null; if (tstd2) tstd2.scrollIntoView({ block: 'nearest', inline: 'nearest' }); }
      } break;
      case 'createcanvascontainer': {
        var ccdiv = document.createElement('div');
        ccdiv.style.position = 'absolute'; ccdiv.style.boxSizing = 'border-box'; ccdiv.style.margin = '0'; ccdiv.style.overflow = 'hidden';
        var cccv = document.createElement('canvas');
        cccv.style.position = 'absolute'; cccv.style.left = '0'; cccv.style.top = '0';
        cccv.style.width = '100%'; cccv.style.height = '100%'; cccv.style.pointerEvents = 'none';
        ccdiv.appendChild(cccv); ccdiv.__iupCanvas = cccv;
        els[c.id] = ccdiv; ccdiv.dataset.iupId = c.id;
        // 2D container canvas: rendered on the worker, blitted to cccv (no transfer)
      } break;
      case 'canvasscrollable': {
        if (el) { el.style.overflow = 'auto'; if (el.__iupCanvas) el.__iupCanvas.style.display = 'none'; }
      } break;
      case 'canvassetup': {
        if (el) {
          var csid = c.id;
          el.style.display = 'block'; el.tabIndex = 0; el.style.outline = 'none';
          var csro = new ResizeObserver(function () { var r = el.getBoundingClientRect(); D('iupwasmCanvasOnResize', csid, Math.round(r.width), Math.round(r.height)); });
          csro.observe(el); el.__iupRO = csro;
          el.addEventListener('mousemove', function (e) { D('iupwasmCanvasMotion', csid, Math.round(e.offsetX), Math.round(e.offsetY), mmods(e)); });
          el.addEventListener('wheel', function (e) { D('iupwasmCanvasWheel', csid, e.deltaY < 0 ? 1 : -1, Math.round(e.offsetX), Math.round(e.offsetY)); e.preventDefault(); }, { passive: false });

          // raw touch + semantic gesture recognition (touch pointers only; mouse stays BUTTON/MOTION/WHEEL)
          var tpts = {}, g1 = null, g2 = null, lpTimer = null;
          var cxy = function (e) { var r = el.getBoundingClientRect(); return [Math.round(e.clientX - r.left), Math.round(e.clientY - r.top)]; };
          var tlist = function () { return Object.keys(tpts); };
          var fireG = function (g, s, x, y, v1, v2) { Dt('iupwasmCanvasGesture', ['number', 'number', 'number', 'number', 'number', 'number', 'number'], [csid, g, s, x, y, v1, v2]); };
          var fireTouch = function (pid, x, y, st) { Dt('iupwasmCanvasTouch', ['number', 'number', 'number', 'number', 'string'], [csid, pid, x, y, st]); };
          var fireMulti = function (changedId, chState) {
            var ks = tlist(); if (!ks.length) return;
            var s = ks.map(function (k) { var t = tpts[k]; return k + ',' + t.x + ',' + t.y + ',' + (k === String(changedId) ? chState : 77); }).join(';');
            Dt('iupwasmCanvasMultiTouch', ['number', 'number', 'string'], [csid, ks.length, s]);
          };
          el.addEventListener('pointerdown', function (e) {
            if (e.pointerType !== 'touch') return;
            var xy = cxy(e), n0 = tlist().length;
            tpts[e.pointerId] = { x: xy[0], y: xy[1] };
            try { el.setPointerCapture(e.pointerId); } catch (_) {}
            fireTouch(e.pointerId, xy[0], xy[1], n0 === 0 ? 'DOWN-PRIMARY' : 'DOWN');
            fireMulti(e.pointerId, 68);
            if (n0 === 0) {
              g1 = { id: e.pointerId, sx: xy[0], sy: xy[1], st: performance.now(), moved: false, lp: false };
              lpTimer = setTimeout(function () { if (g1 && !g1.moved) { g1.lp = true; fireG(5, 2, g1.sx, g1.sy, 0, 0); } }, 500);
            } else if (n0 === 1) {
              clearTimeout(lpTimer); g1 = null;
              var ks = tlist(), a = tpts[ks[0]], b = tpts[ks[1]], dx = b.x - a.x, dy = b.y - a.y;
              g2 = { d0: Math.hypot(dx, dy) || 1, ang0: Math.atan2(dy, dx), cx0: (a.x + b.x) / 2, cy0: (a.y + b.y) / 2 };
              var cx = (a.x + b.x) / 2 | 0, cy = (a.y + b.y) / 2 | 0;
              fireG(0, 0, cx, cy, 1, 0); fireG(1, 0, cx, cy, 0, 0); fireG(2, 0, cx, cy, 0, 0);
            }
          });
          el.addEventListener('pointermove', function (e) {
            if (e.pointerType !== 'touch' || !tpts[e.pointerId]) return;
            var xy = cxy(e); tpts[e.pointerId].x = xy[0]; tpts[e.pointerId].y = xy[1];
            var ks = tlist();
            fireTouch(e.pointerId, xy[0], xy[1], ks[0] === String(e.pointerId) ? 'MOVE-PRIMARY' : 'MOVE');
            fireMulti(e.pointerId, 77);
            if (ks.length >= 2 && g2) {
              var a = tpts[ks[0]], b = tpts[ks[1]], dx = b.x - a.x, dy = b.y - a.y;
              var cx = (a.x + b.x) / 2, cy = (a.y + b.y) / 2;
              fireG(0, 1, cx | 0, cy | 0, Math.hypot(dx, dy) / g2.d0, 0);
              fireG(1, 1, cx | 0, cy | 0, (Math.atan2(dy, dx) - g2.ang0) * 180 / Math.PI, 0);
              fireG(2, 1, cx | 0, cy | 0, cx - g2.cx0, cy - g2.cy0);
            } else if (ks.length === 1 && g1 && (Math.abs(xy[0] - g1.sx) > 8 || Math.abs(xy[1] - g1.sy) > 8)) {
              g1.moved = true;
            }
          });
          var endPtr = function (e) {
            if (e.pointerType !== 'touch' || !tpts[e.pointerId]) return;
            var xy = [tpts[e.pointerId].x, tpts[e.pointerId].y], wasPrimary = tlist()[0] === String(e.pointerId);
            fireTouch(e.pointerId, xy[0], xy[1], wasPrimary ? 'UP-PRIMARY' : 'UP');
            fireMulti(e.pointerId, 85);
            delete tpts[e.pointerId];
            if (g2 && tlist().length < 2) { fireG(0, 2, xy[0], xy[1], 1, 0); fireG(1, 2, xy[0], xy[1], 0, 0); fireG(2, 2, xy[0], xy[1], 0, 0); g2 = null; }
            if (g1 && e.pointerId === g1.id) {
              clearTimeout(lpTimer);
              if (!g1.lp) {
                var dt = performance.now() - g1.st, ddx = xy[0] - g1.sx, ddy = xy[1] - g1.sy, dist = Math.hypot(ddx, ddy);
                if (dist < 10 && dt < 300) {
                  var now = performance.now(), cnt = (el.__iupLastTap && now - el.__iupLastTap < 350) ? 2 : 1;
                  el.__iupLastTap = cnt === 2 ? 0 : now;
                  fireG(4, 2, xy[0], xy[1], cnt, 0);
                } else if (dt < 400 && dist > 30) {
                  fireG(3, 2, xy[0], xy[1], Math.abs(ddx) > Math.abs(ddy) ? (ddx > 0 ? 0 : 1) : (ddy > 0 ? 3 : 2), 0);
                }
              }
              g1 = null;
            }
          };
          el.addEventListener('pointerup', endPtr);
          el.addEventListener('pointercancel', endPtr);
        }
      } break;
      case 'createscrollcanvas': {
        var scwrap = document.createElement('div');
        scwrap.style.position = 'absolute'; scwrap.style.boxSizing = 'border-box'; scwrap.style.margin = '0';
        scwrap.style.overflowX = c.sbH ? 'scroll' : 'hidden'; scwrap.style.overflowY = c.sbV ? 'scroll' : 'hidden';
        var sccontent = document.createElement('div');
        sccontent.style.position = 'relative'; sccontent.style.width = '1px'; sccontent.style.height = '1px';
        var sccv = document.createElement('canvas');
        sccv.style.position = 'sticky'; sccv.style.left = '0'; sccv.style.top = '0'; sccv.style.display = 'block';
        sccontent.appendChild(sccv); scwrap.appendChild(sccontent);
        scwrap.__iupCanvas = sccv; scwrap.__iupContent = sccontent;
        els[c.id] = scwrap; scwrap.dataset.iupId = c.id;
        // 2D scroll canvas: rendered on the worker, blitted to sccv (no transfer)
      } break;
      case 'scrollcanvasrange': {
        if (el && el.__iupContent) { el.__iupContent.style.width = (c.w > 0 ? c.w : 1) + 'px'; el.__iupContent.style.height = (c.h > 0 ? c.h : 1) + 'px'; }
      } break;
      case 'scrollcanvasborder': {
        if (el) el.style.border = '1px solid var(--iup-bd)';
      } break;
      case 'scrollcanvassetpos': {
        if (el) { if (Math.round(el.scrollLeft) !== c.x) el.scrollLeft = c.x; if (Math.round(el.scrollTop) !== c.y) el.scrollTop = c.y; }
      } break;
      case 'scrollcanvasline': {
        if (el) { el.__iupLineX = c.x; el.__iupLineY = c.y; }
      } break;
      case 'scrollcanvassetup': {
        if (el) {
          var scid = c.id, sccv2 = el.__iupCanvas;
          sccv2.tabIndex = 0; sccv2.style.outline = 'none';
          var scro = new ResizeObserver(function () { sccv2.style.width = el.clientWidth + 'px'; sccv2.style.height = el.clientHeight + 'px'; D('iupwasmCanvasOnResize', scid, el.clientWidth, el.clientHeight); });
          scro.observe(el); el.__iupRO = scro;
          el.addEventListener('scroll', function () { D('iupwasmCanvasScroll', scid, Math.round(el.scrollLeft), Math.round(el.scrollTop)); });
          // honor LINEX/LINEY: scroll by the per-step delta instead of the browser default
          el.addEventListener('wheel', function (e) {
            var sx = el.__iupLineX || 0, sy = el.__iupLineY || 0;
            if (!sx && !sy) return;
            e.preventDefault();
            if (e.deltaY && sy) el.scrollTop += (e.deltaY > 0 ? 1 : -1) * sy;
            if (e.deltaX && sx) el.scrollLeft += (e.deltaX > 0 ? 1 : -1) * sx;
          }, { passive: false });
          sccv2.addEventListener('mousemove', function (e) { D('iupwasmCanvasMotion', scid, Math.round(e.offsetX), Math.round(e.offsetY), mmods(e)); });
        }
      } break;
      case 'clipwrite': {
        globalThis.__iupClipText = c.text;
        if (navigator.clipboard && navigator.clipboard.writeText) navigator.clipboard.writeText(c.text).catch(function () {});
      } break;
      case 'openurl': {
        window.open(c.url, '_blank');
      } break;
      case 'download': {
        var dblob = new Blob([c.bytes], { type: c.mime });
        var durl = URL.createObjectURL(dblob);
        var da = document.createElement('a');
        da.href = durl; da.download = c.name || 'download';
        document.body.appendChild(da); da.click(); document.body.removeChild(da);
        setTimeout(function () { URL.revokeObjectURL(durl); }, 1000);
      } break;
      case 'notifyshow': {
        if (typeof Notification !== 'undefined') {
          if (!globalThis.__iupNotify) globalThis.__iupNotify = { map: {} };
          var nmake = function () {
            var n = new Notification(c.title, { body: c.body, silent: !!c.silent });
            globalThis.__iupNotify.map[c.id] = n;
            n.onclick = function () { D('iupwasmNotifyEvent', c.ihptr, c.id, 0); };
            n.onclose = function () { D('iupwasmNotifyEvent', c.ihptr, c.id, 1); delete globalThis.__iupNotify.map[c.id]; };
            n.onerror = function () { D('iupwasmNotifyEvent', c.ihptr, c.id, 2); };
          };
          if (Notification.permission === 'granted') nmake();
          else if (Notification.permission !== 'denied') Notification.requestPermission().then(function (p) { if (p === 'granted') nmake(); });
        }
      } break;
      case 'notifyclose': {
        var nm = globalThis.__iupNotify;
        if (nm && nm.map[c.id]) { nm.map[c.id].close(); delete nm.map[c.id]; }
      } break;
      case 'dragsource': {
        if (el) {
          el.draggable = !!c.enable;
          if (c.enable && !el.__iupDragWired) {
            el.__iupDragWired = 1;
            var dsid = c.id;
            el.addEventListener('dragstart', function (e) {
              e.dataTransfer.setData('text/plain', el.dataset.iupDragType || 'IUPDRAG'); e.dataTransfer.effectAllowed = 'copyMove';
              D('iupwasmDndDragBegin', dsid, Math.round(e.offsetX || 0), Math.round(e.offsetY || 0));
            });
            el.addEventListener('dragend', function (e) { D('iupwasmDndDragEnd', dsid, e.dataTransfer.dropEffect === 'move' ? 1 : 0); });
          }
        }
      } break;
      case 'droptarget': {
        if (el && c.enable && !el.__iupDropWired) {
          el.__iupDropWired = 1;
          var dtid = c.id;
          el.addEventListener('dragover', function (e) { e.preventDefault(); D('iupwasmDndDropMotion', dtid, Math.round(e.offsetX || 0), Math.round(e.offsetY || 0)); });
          el.addEventListener('drop', function (e) {
            e.preventDefault();
            var s = e.dataTransfer.getData('text/plain') || '';
            Dt('iupwasmDndDrop', ['number', 'string', 'number', 'number', 'number'], [dtid, s, new TextEncoder().encode(s).length, Math.round(e.offsetX || 0), Math.round(e.offsetY || 0)]);
          });
        }
      } break;
      case 'dropfilestarget': {
        if (el && !el.__iupDropFilesWired) {
          el.__iupDropFilesWired = 1;
          var dftid = c.id;
          el.addEventListener('dragover', function (e) { e.preventDefault(); });
          el.addEventListener('drop', function (e) {
            if (!e.dataTransfer.files || !e.dataTransfer.files.length) return;
            e.preventDefault();
            var dfx = Math.round(e.offsetX || 0), dfy = Math.round(e.offsetY || 0), files = e.dataTransfer.files;
            for (var i = 0; i < files.length; i++) Dt('iupwasmDndDropFile', ['number', 'string', 'number', 'number', 'number'], [dftid, files[i].name, files.length - 1 - i, dfx, dfy]);
          });
        }
      } break;
      case 'websetup': {
        if (el) {
          var wid = c.id;
          el.style.border = 'none'; el.style.background = 'var(--iup-txtbg)'; el.setAttribute('allowfullscreen', '');
          el.addEventListener('load', function () { D('iupwasmWebOnLoad', wid); });
        }
      } break;
      case 'webseturl': {
        if (el) { el.removeAttribute('srcdoc'); el.src = c.url; }
      } break;
      case 'websethtml': {
        if (el) { el.removeAttribute('src'); el.srcdoc = c.html; }
      } break;
      case 'webreload': {
        if (el) { try { el.contentWindow.location.reload(); } catch (e) { if (el.src) el.src = el.src; } }
      } break;
      case 'webstop': {
        if (el) { try { el.contentWindow.stop(); } catch (e) {} }
      } break;
      case 'webgo': {
        if (el) { try { el.contentWindow.history.go(c.n); } catch (e) {} }
      } break;
      case 'webprint': {
        if (el) { try { el.contentWindow.focus(); el.contentWindow.print(); } catch (e) {} }
      } break;
      case 'webzoom': {
        if (el) el.style.zoom = (c.percent / 100);
      } break;
      case 'webdesignmode': {
        if (el) { try { var d = el.contentDocument; d.designMode = c.on ? 'on' : 'off'; if (c.on) { el.__iupDirty = false; d.addEventListener('input', function () { el.__iupDirty = true; }); } } catch (e) {} }
      } break;
      case 'webcleardirty': {
        if (el) el.__iupDirty = false;
      } break;
      case 'webexec': {
        if (el) { try { el.contentDocument.execCommand(c.cmd, false, c.val != null ? c.val : null); } catch (e) {} }
      } break;
      case 'websetinner': {
        if (el) { try { var n = el.contentDocument.getElementById(c.elemId); if (n) n.innerText = c.text; } catch (e) {} }
      } break;
      case 'websetattr': {
        if (el) { try { var n = el.contentDocument.getElementById(c.elemId); if (n) n.setAttribute(c.name, c.val); } catch (e) {} }
      } break;
      case 'webfind': {
        if (el) { try { var t = window.prompt('Find:'); if (t) el.contentWindow.find(t); } catch (e) {} }
      } break;
      case 'treecreate': {
        if (!globalThis.__iupTree) globalThis.__iupTree = { nodes: {}, next: 1 };
        var te = document.createElement('div');
        te.style.position = 'absolute'; te.style.boxSizing = 'border-box'; te.style.overflow = 'auto';
        te.style.border = '1px solid var(--iup-bd)'; te.style.background = 'var(--iup-txtbg)'; te.tabIndex = 0; te.style.outline = 'none';
        els[c.id] = te; te.dataset.iupId = c.id; te.__iupTreeFocus = 0;
      } break;
      case 'treedndsrc':
      case 'treedndtgt': {
        var tdt = els[c.id];
        if (tdt) {
          if (c.op === 'treedndsrc') { tdt.__iupDndSrc = !!c.on; for (var tdi = 0; tdi < tdt.children.length; tdi++) tdt.children[tdi].draggable = !!c.on; }
          if (c.op === 'treedndtgt') tdt.__iupDndDrop = !!c.on;
          if (!tdt.__iupTreeDndWired) {
            tdt.__iupTreeDndWired = 1; var tdid = c.id;
            tdt.addEventListener('dragstart', function (e) {
              if (!tdt.__iupDndSrc) return;
              globalThis.__iupTreeDnd = { srcId: tdid, srcY: Math.round(e.clientY - tdt.getBoundingClientRect().top) };
              if (e.dataTransfer) { e.dataTransfer.effectAllowed = 'copyMove'; e.dataTransfer.setData('text/plain', 'IUPTREE'); }
            });
            tdt.addEventListener('dragover', function (e) { if (tdt.__iupDndDrop) { e.preventDefault(); if (e.dataTransfer) e.dataTransfer.dropEffect = e.ctrlKey ? 'copy' : 'move'; } });
            tdt.addEventListener('drop', function (e) {
              if (!tdt.__iupDndDrop) return;
              e.preventDefault();
              var s = globalThis.__iupTreeDnd; globalThis.__iupTreeDnd = null;
              if (s) D('iupwasmDndTransfer', s.srcId, tdid, s.srcY, Math.round(e.clientY - tdt.getBoundingClientRect().top));
            });
          }
        }
      } break;
      case 'treereflow': {
        var tree = els[c.id];
        if (tree) {
          var rows = tree.children, hideBelowDepth = 1e9;
          for (var i = 0; i < rows.length; i++) {
            var r = rows[i];
            if (r.__iupDepth > hideBelowDepth) { r.style.display = 'none'; continue; }
            r.style.display = 'flex'; hideBelowDepth = 1e9;
            if (r.__iupKind === 0 && !r.__iupExpanded) hideBelowDepth = r.__iupDepth;
          }
        }
      } break;
      case 'treeaddnode': {
        var tree = els[c.id];
        if (tree) {
          var T = globalThis.__iupTree;
          var treeId = c.id, rowId = c.rowId;
          var row = buildTreeRow(tree, treeId, rowId, c.isBranch, c.title);
          var depth, parent, insertBefore = null;
          if (c.kindPrev === -1) { depth = 0; parent = 0; }
          else {
            var prev = T.nodes[c.prevRowId];
            if (c.kindPrev === 0 && c.add) { depth = prev.__iupDepth + 1; parent = c.prevRowId; insertBefore = prev.nextSibling; }
            else {
              depth = prev.__iupDepth; parent = prev.__iupParent;
              var ref = prev, n = prev.nextSibling;
              while (n && n.__iupDepth > prev.__iupDepth) { ref = n; n = n.nextSibling; }
              insertBefore = ref.nextSibling;
            }
          }
          row.__iupDepth = depth; row.__iupParent = parent;
          row.__iupIndent.style.width = (depth * (tree.__iupIndentPx || 16)) + 'px';
          if (tree.__iupSpacing) { row.style.paddingTop = (1 + tree.__iupSpacing) + 'px'; row.style.paddingBottom = (1 + tree.__iupSpacing) + 'px'; }
          if (insertBefore) tree.insertBefore(row, insertBefore); else tree.appendChild(row);
          T.nodes[rowId] = row;
          if (tree.__iupImgLeaf || tree.__iupImgCollapsed || tree.__iupImgExpanded) treeNodeIcon(row);
          apply({ op: 'treereflow', id: treeId });
        }
      } break;
      case 'treeindent': {
        var tin = els[c.id];
        if (tin) { tin.__iupIndentPx = c.px; var tinr = tin.children; for (var i = 0; i < tinr.length; i++) if (tinr[i].__iupIndent) tinr[i].__iupIndent.style.width = (tinr[i].__iupDepth * c.px) + 'px'; }
      } break;
      case 'treespacing': {
        var tsp = els[c.id];
        if (tsp) { tsp.__iupSpacing = c.px; var tspr = tsp.children; for (var i = 0; i < tspr.length; i++) { tspr[i].style.paddingTop = (1 + c.px) + 'px'; tspr[i].style.paddingBottom = (1 + c.px) + 'px'; } }
      } break;
      case 'treetopitem': {
        var tti = els[c.id], ttir = globalThis.__iupTree.nodes[c.rowId];
        if (tti && ttir) tti.scrollTop = ttir.offsetTop;
      } break;
      case 'treetitlefont': {
        var ttf = globalThis.__iupTree.nodes[c.rowId];
        if (ttf && ttf.__iupTitleEl) ttf.__iupTitleEl.style.font = c.css;
      } break;
      case 'treesettitle': {
        var tr = globalThis.__iupTree.nodes[c.rowId];
        if (tr) tr.__iupTitleEl.textContent = c.title;
      } break;
      case 'treecolor': {
        var tcr = globalThis.__iupTree.nodes[c.rowId];
        if (tcr) tcr.__iupTitleEl.style.color = c.color;
      } break;
      case 'treenodeimage': {
        var tnr = globalThis.__iupTree.nodes[c.rowId];
        if (tnr) { tnr.__iupImg = c.imgId || 0; treeNodeIcon(tnr); }
      } break;
      case 'treenodeimageexp': {
        var tnre = globalThis.__iupTree.nodes[c.rowId];
        if (tnre) { tnre.__iupImgExpanded = c.imgId || 0; treeNodeIcon(tnre); }
      } break;
      case 'treedefimage': {
        var tdi = els[c.id];
        if (tdi) {
          if (c.which === 0) tdi.__iupImgLeaf = c.imgId || 0;
          else if (c.which === 1) tdi.__iupImgCollapsed = c.imgId || 0;
          else tdi.__iupImgExpanded = c.imgId || 0;
          var tdir = tdi.children; for (var i = 0; i < tdir.length; i++) treeNodeIcon(tdir[i]);
        }
      } break;
      case 'treeexpandall': {
        var tea = els[c.id];
        if (tea) {
          var trows = tea.children;
          for (var ti = 0; ti < trows.length; ti++) {
            var tr2 = trows[ti];
            if (tr2.__iupKind === 0) { tr2.__iupExpanded = c.expand ? 1 : 0; if (tr2.__iupExpEl) tr2.__iupExpEl.textContent = c.expand ? String.fromCharCode(0x25BE) : String.fromCharCode(0x25B8); treeNodeIcon(tr2); }
          }
          apply({ op: 'treereflow', id: c.id });
        }
      } break;
      case 'treesetstate': {
        var tr = globalThis.__iupTree.nodes[c.rowId];
        if (tr && tr.__iupKind === 0) {
          tr.__iupExpanded = c.expanded;
          tr.__iupExpEl.textContent = c.expanded ? String.fromCharCode(0x25BE) : String.fromCharCode(0x25B8);
          treeNodeIcon(tr);
          apply({ op: 'treereflow', id: c.id });
        }
      } break;
      case 'treefocus': {
        var tree = els[c.id];
        if (tree) {
          var pf = globalThis.__iupTree.nodes[tree.__iupTreeFocus];
          if (pf && !pf.__iupMarked) pf.style.background = '';
          var tr = globalThis.__iupTree.nodes[c.rowId];
          if (tr) { if (!tr.__iupMarked) tr.style.background = 'var(--iup-accent-bg)'; tr.scrollIntoView({ block: 'nearest' }); }
          tree.__iupTreeFocus = c.rowId;
        }
      } break;
      case 'treemark': {
        var tmk = els[c.id], tmr = globalThis.__iupTree.nodes[c.rowId];
        if (tmr) { tmr.__iupMarked = c.on ? 1 : 0; tmr.style.background = (c.on || (tmk && tmk.__iupTreeFocus === c.rowId)) ? 'var(--iup-accent-bg)' : ''; }
      } break;
      case 'treemarkclear': {
        var tmc = els[c.id];
        if (tmc) { var tmch = tmc.children; for (var i = 0; i < tmch.length; i++) { tmch[i].__iupMarked = 0; tmch[i].style.background = (tmc.__iupTreeFocus === tmch[i].__iupRowId) ? 'var(--iup-accent-bg)' : ''; } }
      } break;
      case 'treeshowtoggle': {
        var tsht = els[c.id]; if (tsht) tsht.__iupShowToggle = c.mode;
      } break;
      case 'treetogglevalue': {
        var ttvr = globalThis.__iupTree.nodes[c.rowId];
        if (ttvr && ttvr.__iupChk) { if (c.state === -1) ttvr.__iupChk.indeterminate = true; else { ttvr.__iupChk.indeterminate = false; ttvr.__iupChk.checked = c.state === 1; } }
      } break;
      case 'treetogglevisible': {
        var ttvisr = globalThis.__iupTree.nodes[c.rowId];
        if (ttvisr && ttvisr.__iupChk) ttvisr.__iupChk.style.display = c.on ? '' : 'none';
      } break;
      case 'treecopymove': {
        var T = globalThis.__iupTree;
        var srcTree = els[c.srcTree], dstTree = els[c.dstTree];
        var src = T.nodes[c.srcRowId], dst = T.nodes[c.dstRowId];
        if (srcTree && dstTree && src && dst) {
          var sub = [src], sn = src.nextElementSibling;
          while (sn && sn.__iupDepth > src.__iupDepth) { sub.push(sn); sn = sn.nextElementSibling; }
          var delta = (dst.__iupDepth + (c.asChild ? 1 : 0)) - src.__iupDepth;
          var insertBefore;
          if (c.asChild) insertBefore = dst.nextElementSibling;
          else { var dn = dst.nextElementSibling; while (dn && dn.__iupDepth > dst.__iupDepth) dn = dn.nextElementSibling; insertBefore = dn; }
          var indentPx = dstTree.__iupIndentPx || 16;
          if (c.isCopy) {
            var map = {};
            for (var i = 0; i < sub.length; i++) {
              var s = sub[i], nrId = c.base + i;
              var nr = buildTreeRow(dstTree, c.dstTree, nrId, s.__iupKind === 0, s.__iupTitleEl.textContent);
              nr.__iupDepth = s.__iupDepth + delta;
              nr.__iupExpanded = s.__iupExpanded;
              if (s.__iupKind === 0) nr.__iupExpEl.textContent = s.__iupExpanded ? String.fromCharCode(0x25BE) : String.fromCharCode(0x25B8);
              nr.__iupParent = (i === 0) ? (c.asChild ? c.dstRowId : dst.__iupParent) : (map[s.__iupParent] || 0);
              nr.__iupImg = s.__iupImg || 0; nr.__iupImgExpanded = s.__iupImgExpanded || 0;
              nr.__iupTitleEl.style.color = s.__iupTitleEl.style.color; nr.__iupTitleEl.style.font = s.__iupTitleEl.style.font;
              nr.__iupIndent.style.width = (nr.__iupDepth * indentPx) + 'px';
              if (dstTree.__iupSpacing) { nr.style.paddingTop = (1 + dstTree.__iupSpacing) + 'px'; nr.style.paddingBottom = (1 + dstTree.__iupSpacing) + 'px'; }
              treeNodeIcon(nr);
              map[s.__iupRowId] = nrId;
              if (insertBefore) dstTree.insertBefore(nr, insertBefore); else dstTree.appendChild(nr);
              T.nodes[nrId] = nr;
            }
          } else {
            for (var i = 0; i < sub.length; i++) {
              var s = sub[i];
              s.__iupDepth += delta;
              if (i === 0) s.__iupParent = c.asChild ? c.dstRowId : dst.__iupParent;
              s.__iupIndent.style.width = (s.__iupDepth * indentPx) + 'px';
              if (insertBefore) dstTree.insertBefore(s, insertBefore); else dstTree.appendChild(s);
            }
          }
          apply({ op: 'treereflow', id: c.dstTree });
          if (c.srcTree !== c.dstTree) apply({ op: 'treereflow', id: c.srcTree });
        }
      } break;
      case 'treedel': {
        var T = globalThis.__iupTree, tr = T.nodes[c.rowId];
        if (tr) {
          var drows = [tr], dn = tr.nextSibling;
          while (dn && dn.__iupDepth > tr.__iupDepth) { drows.push(dn); dn = dn.nextSibling; }
          for (var i = 0; i < drows.length; i++) { delete T.nodes[drows[i].__iupRowId]; drows[i].remove(); }
          apply({ op: 'treereflow', id: c.id });
        }
      } break;
      case 'treestartrename': {
        var tr = globalThis.__iupTree.nodes[c.rowId];
        if (tr) {
          var treeId = c.id, rowId = c.rowId;
          var rttl = tr.__iupTitleEl, old = rttl.textContent;
          var inp = document.createElement('input');
          inp.value = old; inp.style.font = 'inherit'; inp.style.width = '10em';
          rttl.textContent = ''; rttl.appendChild(inp);
          inp.focus(); inp.select();
          var done = 0;
          var commit = function () {
            if (done) return; done = 1;
            var accept = Dr('iupwasmTreeRenameEnd', 'number', ['number', 'number', 'string'], [treeId, rowId, inp.value]);
            rttl.textContent = accept ? inp.value : old;
          };
          inp.addEventListener('blur', commit);
          inp.addEventListener('keydown', function (e) {
            if (e.key === 'Enter') { e.preventDefault(); inp.blur(); }
            else if (e.key === 'Escape') { done = 1; rttl.textContent = old; }
            e.stopPropagation();
          });
        }
      } break;
      }
    };
    apply.__iupReal = true;
    globalThis.__iupApply = apply;

    if (typeof window !== 'undefined' && !globalThis.__iupResizeWired) {
      globalThis.__iupResizeWired = 1;
      window.addEventListener('resize', function () {
        var vw = window.innerWidth, vh = window.innerHeight;
        for (var id in els) {
          var de = els[id];
          if (!de || !de.__iupViewportFill || de.style.display === 'none') continue;
          de.style.width = vw + 'px'; de.style.height = vh + 'px';
          D('iupwasmDialogResize', parseInt(id), vw, vh);
        }
      });
    }

    globalThis.__iupRead = function (req) {
      var el = els[req.id];
      switch (req.op) {
      case 'elemrect': {
        if (!el) return [0, 0, 0, 0];
        var r = el.getBoundingClientRect();
        return [Math.round(r.left), Math.round(r.top), Math.round(r.width), Math.round(r.height)];
      } break;
      case 'elemscreen': {
        if (!el) return [0, 0];
        var rs = el.getBoundingClientRect();
        return [Math.round(rs.left + (window.screenX || 0)), Math.round(rs.top + (window.screenY || 0))];
      } break;
      case 'measuremarkup': {
        var mm = globalThis.__iupMarkupProbe;
        if (!mm) {
          mm = document.createElement('span');
          mm.style.position = 'absolute'; mm.style.visibility = 'hidden'; mm.style.whiteSpace = 'nowrap';
          document.body.appendChild(mm); globalThis.__iupMarkupProbe = mm;
        }
        mm.style.font = req.css; mm.innerHTML = req.html;
        return [mm.offsetWidth, mm.offsetHeight || 16];
      } break;
      case 'inputvalue': {
        return el ? (el.value || '') : '';
      } break;
      case 'listxy2pos': {
        var lxs = el && el.__iupOpts;
        if (!lxs || !lxs.options || !lxs.options.length) return 0;
        var lvy = lxs.getBoundingClientRect().top + req.y;
        for (var lxi = 0; lxi < lxs.options.length; lxi++) if (lvy <= lxs.options[lxi].getBoundingClientRect().bottom) return lxi + 1;
        return lxs.options.length;
      } break;
      case 'treexy2row': {
        if (!el) return 0;
        var txvy = el.getBoundingClientRect().top + req.y, txlast = 0;
        for (var txi = 0; txi < el.children.length; txi++) {
          var txr = el.children[txi];
          if (txr.style.display === 'none') continue;
          txlast = txr.__iupRowId;
          if (txvy <= txr.getBoundingClientRect().bottom) return txr.__iupRowId;
        }
        return txlast;
      } break;
      case 'togglevalue': {
        if (el && el.__iupInput && el.__iupInput.indeterminate) return -1;
        return (el && el.__iupInput && el.__iupInput.checked) ? 1 : 0;
      } break;
      case 'viewport': {
        return [window.innerWidth || 800, window.innerHeight || 600];
      } break;
      case 'cursorpos': {
        var cm = globalThis.__iupMouse || [0, 0];
        return [Math.round(cm[0] + (window.screenX || 0)), Math.round(cm[1] + (window.screenY || 0))];
      } break;
      case 'keystate': {
        return globalThis.__iupMods || 0;
      } break;
      case 'fullsize': {
        return [screen.width || window.innerWidth || 800, screen.height || window.innerHeight || 600];
      } break;
      case 'screendpi': {
        return Math.round((window.devicePixelRatio || 1) * 96);
      } break;
      case 'virtualscreen': {
        return [(screen.availLeft || 0) | 0, (screen.availTop || 0) | 0, screen.width || 800, screen.height || 600];
      } break;
      case 'dlgisactive': {
        return (document.hasFocus() && globalThis.__iupActiveDialog === req.id) ? 1 : 0;
      } break;
      case 'darkmode': {
        return (typeof window !== 'undefined' && window.matchMedia && window.matchMedia('(prefers-color-scheme: dark)').matches) ? 1 : 0;
      } break;
      case 'treedepth': {
        var tdn = globalThis.__iupTree && globalThis.__iupTree.nodes[req.rowId]; return tdn ? tdn.__iupDepth : 0;
      } break;
      case 'treechildcount': {
        var tcn = globalThis.__iupTree && globalThis.__iupTree.nodes[req.rowId]; if (!tcn) return 0;
        var tcs = tcn.nextSibling, tcc = 0; while (tcs && tcs.__iupDepth > tcn.__iupDepth) { tcc++; tcs = tcs.nextSibling; } return tcc;
      } break;
      case 'treetitle': {
        var ttn = globalThis.__iupTree && globalThis.__iupTree.nodes[req.rowId]; return ttn ? ttn.__iupTitleEl.textContent : '';
      } break;
      case 'treestate': {
        var tsn = globalThis.__iupTree && globalThis.__iupTree.nodes[req.rowId]; if (!tsn || tsn.__iupKind !== 0) return -1; return tsn.__iupExpanded ? 1 : 0;
      } break;
      case 'treefocus': {
        var tfe = els[req.id]; return tfe ? (tfe.__iupTreeFocus || 0) : 0;
      } break;
      case 'treenav': {
        var tnr = globalThis.__iupTree && globalThis.__iupTree.nodes[req.rowId]; if (!tnr) return 0;
        if (req.mode === 0) return tnr.__iupParent || 0;
        if (req.mode === 1) { var tnn = tnr.nextElementSibling; while (tnn) { if (tnn.__iupDepth < tnr.__iupDepth) return 0; if (tnn.__iupDepth === tnr.__iupDepth) return tnn.__iupRowId; tnn = tnn.nextElementSibling; } return 0; }
        if (req.mode === 2) { var tnp = tnr.previousElementSibling; while (tnp) { if (tnp.__iupDepth < tnr.__iupDepth) return 0; if (tnp.__iupDepth === tnr.__iupDepth) return tnp.__iupRowId; tnp = tnp.previousElementSibling; } return 0; }
        if (req.mode === 3) { if (tnr.__iupDepth === 0) return tnr.parentNode.firstElementChild ? tnr.parentNode.firstElementChild.__iupRowId : 0; var tnpr = globalThis.__iupTree.nodes[tnr.__iupParent]; return (tnpr && tnpr.nextElementSibling) ? tnpr.nextElementSibling.__iupRowId : 0; }
        if (req.mode === 4) { var tnl = tnr, tnm = tnr.nextElementSibling; while (tnm) { if (tnm.__iupDepth < tnr.__iupDepth) break; if (tnm.__iupDepth === tnr.__iupDepth) tnl = tnm; tnm = tnm.nextElementSibling; } return tnl.__iupRowId; }
        return 0;
      } break;
      case 'treerootcount': {
        var trc = els[req.id]; if (!trc) return 0; var trcc = 0, trch = trc.children; for (var i = 0; i < trch.length; i++) if (trch[i].__iupDepth === 0) trcc++; return trcc;
      } break;
      case 'treemarked': {
        var tmn = globalThis.__iupTree && globalThis.__iupTree.nodes[req.rowId]; return (tmn && tmn.__iupMarked) ? 1 : 0;
      } break;
      case 'treetogglevalue': {
        var rttvn = globalThis.__iupTree && globalThis.__iupTree.nodes[req.rowId]; if (!rttvn || !rttvn.__iupChk) return 0; if (rttvn.__iupChk.indeterminate) return -1; return rttvn.__iupChk.checked ? 1 : 0;
      } break;
      case 'treetogglevisible': {
        var rttvisn = globalThis.__iupTree && globalThis.__iupTree.nodes[req.rowId]; return (rttvisn && rttvisn.__iupChk && rttvisn.__iupChk.style.display !== 'none') ? 1 : 0;
      } break;
      case 'textvalue': {
        return el ? (el.value || '') : '';
      } break;
      case 'textcaret': {
        return el ? (el.selectionStart || 0) : 0;
      } break;
      case 'textselpos': {
        return el ? ((el.selectionStart || 0) + ':' + (el.selectionEnd || 0)) : '';
      } break;
      case 'textseltext': {
        return el ? el.value.slice(el.selectionStart || 0, el.selectionEnd || 0) : '';
      } break;
      case 'textcount': {
        return el ? el.value.length : 0;
      } break;
      case 'textlctopos': {
        return el ? iupLcToPos(el.value, req.lin, req.col) : (req.col > 0 ? req.col - 1 : 0);
      } break;
      case 'textpostolc': {
        if (!el) return [1, req.pos + 1];
        var tlc = iupPosToLc(el.value, req.pos).split(',');
        return [parseInt(tlc[0]), parseInt(tlc[1])];
      } break;
      case 'textcaretlc': {
        return el ? iupPosToLc(el.value, el.selectionStart || 0) : '1,1';
      } break;
      case 'textsellc': {
        if (!el || (el.selectionStart || 0) === (el.selectionEnd || 0)) return '';
        return iupPosToLc(el.value, el.selectionStart || 0) + ':' + iupPosToLc(el.value, el.selectionEnd || 0);
      } break;
      case 'textlinecount': {
        return el ? el.value.split('\n').length : 1;
      } break;
      case 'textlinevalue': {
        if (!el) return '';
        var lv = el.value.split('\n'); var li = el.value.slice(0, el.selectionStart || 0).split('\n').length - 1;
        return lv[li] || '';
      } break;
      case 'vlistscrolltop': {
        return el ? (el.scrollTop | 0) : 0;
      } break;
      case 'vlistclienth': {
        return el ? (el.clientHeight | 0) : 0;
      } break;
      case 'listcount': {
        return el ? el.__iupOpts.children.length : 0;
      } break;
      case 'listselindex': {
        return (el && el.__iupOpts.selectedIndex !== undefined) ? el.__iupOpts.selectedIndex : -1;
      } break;
      case 'listmulti': {
        var lms = ''; if (el) { var lmo = el.__iupOpts.options; for (var k = 0; k < lmo.length; k++) lms += lmo[k].selected ? '+' : '-'; } return lms;
      } break;
      case 'listtext': {
        return el ? el.__iupVal.value : '';
      } break;
      case 'listeditcaret': {
        return (el && el.__iupVal) ? (el.__iupVal.selectionStart || 0) : 0;
      } break;
      case 'listeditselpos': {
        return (el && el.__iupVal) ? ((el.__iupVal.selectionStart || 0) + ':' + (el.__iupVal.selectionEnd || 0)) : '';
      } break;
      case 'listeditseltext': {
        return (el && el.__iupVal) ? el.__iupVal.value.slice(el.__iupVal.selectionStart || 0, el.__iupVal.selectionEnd || 0) : '';
      } break;
      case 'listitemtext': {
        return (el && el.__iupOpts.children[req.pos]) ? el.__iupOpts.children[req.pos].text : '';
      } break;
      case 'listitemimg': {
        return (el && el.__iupOpts && el.__iupOpts.children[req.pos]) ? (el.__iupOpts.children[req.pos].__iupImgId || 0) : 0;
      } break;
      case 'tablecolwidthget': {
        if (!el) return 0;
        var tcwth = el.__iupHead.children[req.col - 1];
        return tcwth ? tcwth.offsetWidth : 0;
      } break;
      case 'tablegetcell': {
        if (!el) return '';
        var tgr = el.__iupBody.children[req.lin - 1]; if (!tgr) return '';
        var tgtd = tgr.children[req.col - 1]; return tgtd ? (tgtd.__iupText || '') : '';
      } break;
      case 'tablevscrolltop': {
        return el ? (el.scrollTop | 0) : 0;
      } break;
      case 'tablevclienth': {
        return el ? (el.clientHeight | 0) : 0;
      } break;
      case 'canvasclientw': {
        return el ? (el.clientWidth || el.width || 0) : 0;
      } break;
      case 'canvasclienth': {
        return el ? (el.clientHeight || el.height || 0) : 0;
      } break;
      case 'notifyavailable': {
        return (typeof Notification !== 'undefined') ? 1 : 0;
      } break;
      case 'notifypermission': {
        if (typeof Notification === 'undefined') return 2;
        if (Notification.permission === 'granted') return 1;
        if (Notification.permission === 'denied') return 2;
        return 0;
      } break;
      case 'prompt': {
        return window.prompt(req.msg, req.def || '');
      } break;
      case 'fileopensize': {
        return (globalThis.__iupOpenFile && globalThis.__iupOpenFile.bytes) ? globalThis.__iupOpenFile.bytes.length : 0;
      } break;
      case 'modalnext': {
        var mq = globalThis.__iupModalQueue;
        return (mq && mq.length) ? JSON.stringify(mq.shift()) : null;
      } break;
      case 'pumpenter': {
        globalThis.__iupModalActive = (globalThis.__iupModalActive || 0) + 1;
        return 0;
      } break;
      case 'pumpleave': {
        globalThis.__iupModalActive = Math.max(0, (globalThis.__iupModalActive || 0) - 1);
        if (globalThis.__iupModalActive === 0) globalThis.__iupModalQueue = [];
        return 0;
      } break;
      case 'webgethtml': {
        if (!el) return '';
        try { var d = el.contentDocument; if (d) return '<html>' + d.documentElement.innerHTML + '</html>'; } catch (e) {}
        return '';
      } break;
      case 'webgeturl': {
        if (!el) return '';
        var u = ''; try { var w = el.contentWindow; if (w) u = w.location.href; } catch (e) {}
        return u === 'about:blank' ? '' : u;
      } break;
      case 'webdesignmodeget': {
        try { return (el.contentDocument.designMode === 'on') ? 1 : 0; } catch (e) { return 0; }
      } break;
      case 'webdirty': {
        return (el && el.__iupDirty) ? 1 : 0;
      } break;
      case 'webeval': {
        var es = ''; try { var r = el.contentWindow.eval(req.code); if (r !== undefined && r !== null) es = '' + r; } catch (e) { es = '' + e; }
        return es;
      } break;
      case 'webquery': {
        var qs = ''; try { var qd = el.contentDocument; if (req.kind === 0) qs = qd.queryCommandState(req.cmd) ? 'YES' : 'NO'; else if (req.kind === 1) qs = qd.queryCommandEnabled(req.cmd) ? 'YES' : 'NO'; else qs = '' + qd.queryCommandValue(req.cmd); } catch (e) {}
        return qs;
      } break;
      case 'webgetinner': {
        var is = ''; try { var inn = el.contentDocument.getElementById(req.elemId); if (inn) is = inn.innerText; } catch (e) {}
        return is;
      } break;
      case 'webgetattr': {
        var as = ''; try { var an = el.contentDocument.getElementById(req.elemId); if (an) as = an.getAttribute(req.name) || ''; } catch (e) {}
        return as;
      } break;
      }
      return 0;
    };

    globalThis.__iupInstallKeyHandler = function () {
      if (globalThis.__iupKeyInstalled) return;
      globalThis.__iupKeyInstalled = true;
      function iupId(t) {
        var n = t && t.closest ? t.closest('[data-iup-id]') : null;
        return n ? parseInt(n.dataset.iupId) : 0;
      }
      var SP = { Escape: 0xFF1B, Home: 0xFF50, ArrowLeft: 0xFF51, ArrowUp: 0xFF52,
                 ArrowRight: 0xFF53, ArrowDown: 0xFF54, PageUp: 0xFF55, PageDown: 0xFF56,
                 End: 0xFF57, Insert: 0xFF63, Delete: 0xFFFF, Backspace: 8, Tab: 9,
                 Enter: 13, Pause: 0xFF13 };
      // shared globals read by iupdrvGetCursorPos / iupdrvGetKeyState over the SAB
      var stashMods = function (e) {
        globalThis.__iupMods = (e.shiftKey ? 1 : 0) | (e.ctrlKey ? 2 : 0) | (e.altKey ? 4 : 0) | (e.metaKey ? 8 : 0);
      };
      document.addEventListener('keyup', stashMods, true);
      document.addEventListener('keydown', function (e) {
        stashMods(e);
        var id = iupId(document.activeElement) || (globalThis.__iupActiveDialog || 0);
        if (!id) return;
        var k = e.key, code = 0;
        if (Object.prototype.hasOwnProperty.call(SP, k)) code = SP[k];
        else if (k && k.length === 1) code = k.charCodeAt(0);
        else if (/^F([1-9]|1[0-2])$/.test(k)) code = 0xFFBE + (parseInt(k.slice(1)) - 1);
        if (!code) return;
        if (e.ctrlKey) code |= 0x20000000;
        if (e.altKey) code |= 0x40000000;
        if (e.shiftKey && !(k && k.length === 1)) code |= 0x10000000;
        if (dispatch.sync) {
          if (Dr('iupwasmDispatchKey', 'number', ['number', 'number'], [id, code])) e.preventDefault();
        } else {
          D('iupwasmDispatchKey', id, code);
        }
        if (k === 'F1') { e.preventDefault(); D('iupwasmDispatchHelp', id); }  // GTK fires HELP_CB on plain F1
      });
      document.addEventListener('focusin', function (e) { var id = iupId(e.target); if (id) D('iupwasmDispatchFocus', id, 1); });
      document.addEventListener('focusout', function (e) { var id = iupId(e.target); if (id) D('iupwasmDispatchFocus', id, 0); });
      document.addEventListener('mousedown', function (e) {
        stashMods(e);
        var id = iupId(e.target); if (!id) return;
        D('iupwasmDispatchButton', id, 49 + (e.button || 0), 1, Math.round(e.offsetX || 0), Math.round(e.offsetY || 0));
      });
      document.addEventListener('mouseup', function (e) {
        stashMods(e);
        var id = iupId(e.target); if (!id) return;
        D('iupwasmDispatchButton', id, 49 + (e.button || 0), 0, Math.round(e.offsetX || 0), Math.round(e.offsetY || 0));
      });
      // generic MOTION_CB for non-canvas controls (canvas wires its own, avoid double-fire)
      document.addEventListener('mousemove', function (e) {
        stashMods(e);
        globalThis.__iupMouse = [e.clientX, e.clientY];
        if (e.target.tagName === 'CANVAS') return;
        var id = iupId(e.target); if (id) D('iupwasmDispatchMotion', id, Math.round(e.offsetX || 0), Math.round(e.offsetY || 0), mmods(e));
      });
      // ENTERWINDOW_CB / LEAVEWINDOW_CB, delegated: fire only when the resolved iup element changes
      var hoverId = 0;
      var iupHover = function (target) {
        var id = iupId(target);
        if (id === hoverId) return;
        if (hoverId) D('iupwasmDispatchLeave', hoverId);
        hoverId = id;
        if (id) D('iupwasmDispatchEnter', id);
      };
      document.addEventListener('mouseover', function (e) { iupHover(e.target); });
      document.addEventListener('mouseout', function (e) { if (!e.relatedTarget || !iupId(e.relatedTarget)) iupHover(null); });
    };
  };
})();
