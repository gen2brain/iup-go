new PagefindUI({ element: "#search", showSubResults: true, showImages: false, resetStyles: false });

(function () {
  var here = location.pathname.replace(/\/$/, "/index.html");
  var links = document.querySelectorAll(".sidebar-nav a");
  links.forEach(function (a) {
    if (a.pathname === here) {
      a.classList.add("active");
      for (var d = a.closest("details"); d; d = d.parentElement.closest("details")) d.open = true;
      a.scrollIntoView({ block: "center" });
    }
  });

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
