// Simplify the sidebar TOC
window.addEventListener('DOMContentLoaded', function () {
    [].forEach.call(document.querySelectorAll('.toc-h3'), function (el) {
        el.parentElement.remove()
    });
})