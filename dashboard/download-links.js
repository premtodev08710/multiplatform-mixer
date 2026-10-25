// Keep multitrack downloads in a separate browsing tab, including cards loaded later.
document.addEventListener('click', (event) => {
  const link = event.target instanceof Element
    ? event.target.closest('#multitracks a[href]')
    : null;
  if (!link) return;

  link.target = '_blank';
  link.relList.add('noopener', 'noreferrer');
  // Preserve native link navigation and modifier keys; no popup or async redirect.
}, true);
