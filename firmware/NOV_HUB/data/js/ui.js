/* Shared gallery drawing and presentation. No fonts, images or scripts leave the Hub. */
const UI = (() => {
  const escape = value => String(value ?? '').replace(/[&<>"']/g, char => String.fromCharCode(38) + '#' + char.charCodeAt(0) + ';');
  const has = value => typeof value === 'number' && Number.isFinite(value);
  const drawings = {
    terracotta: '<path class="art-fill" d="M61 22h118v116H61z"/><path d="M68 29h104v102H68zM75 36h90v88H75z"/><path d="M120 112V66m0 25c-27 1-34-18-30-30 23 0 31 15 30 30Zm0-13c25 0 31-20 26-30-20 3-28 14-26 30Z"/><path d="M120 65c-23-13-17-28 0-29 17 1 23 16 0 29ZM84 111l9-9 9 9-9 9Zm54 0 9-9 9 9-9 9ZM79 45h5m72 0h5M79 119h5m72 0h5"/>',
    woodenboat: '<path class="art-fill" d="M25 103c57 15 133 15 190-9-16 29-40 41-88 41-49 0-78-11-102-32Z"/><path d="M25 103c57 15 133 15 190-9M45 112c50 13 107 13 151-5M74 108l14 23m9-20 7 23m16-22v23m20-25-6 24m28-28-13 25m32-29-16 22"/><path d="M108 107V29m5 7c31 13 46 35 52 58h-51m-14-55C78 52 64 70 58 91h43M108 29l22 5-22 7"/><path class="art-river" d="M15 143c13-6 25 6 38 0s25 6 38 0 25 6 38 0 25 6 38 0 25 6 38 0 17 1 22 1"/>',
    rickshaw: '<circle cx="67" cy="120" r="25"/><circle cx="176" cy="120" r="25"/><circle cx="67" cy="120" r="5"/><circle cx="176" cy="120" r="5"/><path d="m67 120 33-50 26 50H67l24-35h57l28 35m-50 0 25-58h17m-71 8H84m67-8 4-13h15"/><path class="art-fill" d="M41 71c-5-41 53-58 77-22l-6 32H55Z"/><path d="M49 64c2-27 37-36 59-14M61 67l38-14m-29-4 25 23M55 82v22m57-23v24M73 37v13m18-15-4 14"/><path d="m74 63 7-7 7 7-7 7Z"/>',
    pottery: '<path class="art-fill" d="M93 25h54l-4 17c-2 14 7 20 20 33 33 34 14 69-43 69S43 109 77 75c13-13 22-19 20-33Z"/><ellipse cx="120" cy="25" rx="27" ry="6"/><path d="M96 43h48M83 71c20 9 53 9 74 0M73 86c28 10 65 10 94 0M70 102c27 11 72 11 100 0M85 133c21 5 50 5 70 0"/><path d="m81 94 8 13 9-10 10 13 12-11 12 11 10-13 9 10 8-13"/>',
    flute: '<g transform="rotate(-27 120 80)"><path class="art-fill" d="M28 70h184v23H28Z"/><ellipse cx="28" cy="81.5" rx="5" ry="11.5"/><path d="M42 70v23m5-23v23m144-23v23m5-23v23M212 70c6 4 6 19 0 23"/><circle cx="70" cy="81" r="3"/><circle cx="90" cy="81" r="3"/><circle cx="110" cy="81" r="3"/><circle cx="130" cy="81" r="3"/><circle cx="150" cy="81" r="3"/><circle cx="170" cy="81" r="3"/></g><path class="art-river" d="M58 43c-9-8 2-13-4-21m19 14c-8-8 3-13-2-20m108 111c-8-8 2-13-3-21"/>',
    basket: '<path d="M78 56C78 4 162 4 162 56m-76 0c0-42 68-42 68 0"/><path class="art-fill" d="m49 61 18 70c28 18 78 18 106 0l18-70Z"/><ellipse cx="120" cy="60" rx="71" ry="15"/><ellipse cx="120" cy="60" rx="61" ry="9"/><path d="m61 79 119 18M66 99l109 18M71 119l91 16m18-56L61 97m114 2L66 117m103 2-90 16M71 74l15 64m3-61 12 64m8-62 3 63m18-63-3 63m24-65-12 64m30-67-15 64"/>'
  };
  function artwork(id, className = '') {
    return `<svg class="artifact-art ${className}" viewBox="0 0 240 160" fill="none" stroke="currentColor" stroke-width="1.7" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true">${drawings[id] || ''}</svg>`;
  }
  function route(detailed = false, compact = false) {
    return `<ol class="museum-route${detailed ? ' museum-route--detailed' : ''}${compact ? ' museum-route--compact' : ''}" aria-label="${escape(t('routeTitle'))}">${ARTIFACTS.map(a => `<li class="route-stop ${a.simulated ? 'route-stop--demo' : 'route-stop--real'}" data-station="${a.id}">
      <div class="route-marker"><span class="route-number">0${a.station}</span><span class="route-here">${t('here')}</span></div>
      <div class="route-object">${artwork(a.id)}</div><div class="route-copy"><h3>${escape(a.name[getLang()])}</h3>
      ${detailed ? `<p class="artifact-display">${t('display')} · ${escape(a.display[getLang()])}</p><p class="artifact-desc">${escape(a.desc[getLang()])}</p>` : ''}</div></li>`).join('')}</ol>`;
  }
  function highlight(state) {
    const active = state.connection.status !== 'disconnected' ? state.robot.station : '';
    document.querySelectorAll('[data-station]').forEach(el => {
      const current = el.dataset.station === active;
      el.classList.toggle('is-current', current);
      const markerLabel = el.querySelector('.route-here');
      if (markerLabel) markerLabel.textContent = t(state.robot.status === 'moving' ? 'moving' : 'here');
      if (current) el.setAttribute('aria-current', 'step'); else el.removeAttribute('aria-current');
    });
  }
  function connection(state) {
    const el = document.getElementById('connection');
    if (!el) return;
    const c = state.connection, status = c.status;
    document.body.classList.toggle('is-disconnected', status === 'disconnected');
    el.className = 'connection connection--' + status;
    el.replaceChildren();
    const text = document.createElement('span'); text.className = 'connection-copy'; text.textContent = t(status); el.append(text);
    if (status === 'disconnected' || status === 'connecting') {
      const last = document.createElement('small');
      last.textContent = t('lastUpdate') + ': ' + (c.lastSeen ? new Date(c.lastSeen).toLocaleTimeString(getLang() === 'bn' ? 'bn-BD' : 'en-GB') : t('never'));
      const retry = document.createElement('button'); retry.className = 'connection-retry'; retry.textContent = t('retry'); retry.addEventListener('click', Hub.retry);
      el.append(last, retry);
    }
    document.querySelectorAll('[data-command]').forEach(button => { button.disabled = button.dataset.busy === 'true' || (Hub.isReal && status !== 'connected'); });
  }
  function emergency(state) {
    const el = document.getElementById('emergencySlot'); if (!el) return;
    el.textContent = state.emergency ? t('emergencyMsg') : ''; el.hidden = !state.emergency;
  }
  function robot(state) {
    const current = artifactById(state.robot.station);
    const name = document.getElementById('robotStation'), status = document.getElementById('robotActivity'), next = document.getElementById('robotNext');
    if (name) name.textContent = current ? current.name[getLang()] : t('positionUnknown');
    if (status) status.textContent = current ? (state.robot.status === 'moving' ? t('moving') : state.robot.status === 'narrating' ? t('narrating') : t('robotNow')) + ' · ' + String(current.station).padStart(2, '0') : t('waitingRobot');
    if (next) next.textContent = current ? (ARTIFACTS[current.station] ? ARTIFACTS[current.station].name[getLang()] : t('routeEnd')) : '—';
    const art = document.getElementById('robotArt'); if (art) art.innerHTML = current ? artwork(current.id) : '<span class="unknown-position">—</span>';
    const muted = document.getElementById('muteBadge'); if (muted) { muted.textContent = t('mutedBadge'); muted.hidden = !state.muted; }
    const dot = document.getElementById('robotDot'); if (dot) dot.className = 'status-dot' + (state.robot.status === 'moving' ? ' status-dot--moving' : '');
    highlight(state);
  }
  let observer;
  function reveal() {
    if (!('IntersectionObserver' in window) || matchMedia('(prefers-reduced-motion: reduce)').matches) return;
    if (!observer) observer = new IntersectionObserver(entries => entries.forEach(entry => { if (entry.isIntersecting) { entry.target.classList.add('is-visible'); observer.unobserve(entry.target); } }), { threshold: .07 });
    document.querySelectorAll('.route-stop').forEach(el => { el.classList.add('will-reveal'); observer.observe(el); });
  }
  return { escape, has, artwork, route, highlight, connection, emergency, robot, reveal };
})();
