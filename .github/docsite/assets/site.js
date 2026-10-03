new PagefindUI({ element: "#search", showSubResults: true, showImages: false, resetStyles: false });

(function () {
  var key = "iupdoc-open";
  var open = [];
  try { open = JSON.parse(sessionStorage.getItem(key) || "[]"); } catch (e) {}
  document.querySelectorAll(".sidebar-nav details").forEach(function (d, i) {
    if (open.indexOf(i) >= 0) d.open = true;
    d.addEventListener("toggle", function () {
      var list = [];
      document.querySelectorAll(".sidebar-nav details").forEach(function (x, j) { if (x.open) list.push(j); });
      try { sessionStorage.setItem(key, JSON.stringify(list)); } catch (e) {}
    });
  });

  function page(path) { return path.replace(/\/$/, "/index.html").replace(/\.html$/, ""); }
  var here = page(location.pathname);
  var sidebar = document.querySelector(".sidebar");
  document.querySelectorAll(".sidebar-nav a").forEach(function (a) {
    if (page(a.pathname) !== here) return;
    a.classList.add("active");
    for (var d = a.closest("details"); d; d = d.parentElement.closest("details")) d.open = true;
    var top = a.getBoundingClientRect().top - sidebar.getBoundingClientRect().top + sidebar.scrollTop;
    sidebar.scrollTop = top - (sidebar.clientHeight - a.offsetHeight) / 2;
  });

  function markZoomable(img) {
    var scaled = img.naturalWidth > img.clientWidth + 1 || img.naturalHeight > img.clientHeight + 1;
    img.classList.toggle("zoomable", scaled && !img.closest("a"));
  }
  var images = document.querySelectorAll(".content img");
  var observer = new ResizeObserver(function (entries) {
    entries.forEach(function (e) { markZoomable(e.target); });
  });
  images.forEach(function (img) {
    observer.observe(img);
    img.addEventListener("load", function () { markZoomable(img); });
    img.addEventListener("click", function () {
      if (!img.classList.contains("zoomable")) return;
      var box = document.createElement("div");
      box.className = "lightbox";
      var big = document.createElement("img");
      big.src = img.currentSrc || img.src;
      big.alt = img.alt;
      box.appendChild(big);
      function close() { box.remove(); document.removeEventListener("keydown", onKey); }
      function onKey(e) { if (e.key === "Escape") close(); }
      box.addEventListener("click", close);
      document.addEventListener("keydown", onKey);
      document.body.appendChild(box);
    });
  });

  document.querySelector(".menu-toggle").addEventListener("click", function () {
    document.body.classList.toggle("nav-open");
  });
  document.querySelector(".theme-toggle").addEventListener("click", function () {
    var root = document.documentElement;
    var dark = root.dataset.theme ? root.dataset.theme === "dark" : matchMedia("(prefers-color-scheme: dark)").matches;
    root.dataset.theme = dark ? "light" : "dark";
    try { localStorage.setItem("theme", root.dataset.theme); } catch (e) {}
  });

  var tocLinks = document.querySelectorAll(".toc a");
  if (tocLinks.length && "IntersectionObserver" in window) {
    var map = {};
    tocLinks.forEach(function (a) { map[decodeURIComponent(a.hash.slice(1))] = a; });
    var obs = new IntersectionObserver(function (entries) {
      entries.forEach(function (e) {
        if (e.isIntersecting && map[e.target.id]) {
          tocLinks.forEach(function (a) { a.classList.remove("current"); });
          map[e.target.id].classList.add("current");
        }
      });
    }, { rootMargin: "0px 0px -75% 0px" });
    document.querySelectorAll(".content h2[id], .content h3[id], .content h4[id]").forEach(function (h) { obs.observe(h); });
  }
})();
