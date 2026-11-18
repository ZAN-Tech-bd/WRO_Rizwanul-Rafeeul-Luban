/* Page behavior; all hardware access stays in hub.js. */
(() => {
  'use strict';
  const page = document.body.dataset.page;
  const $ = id => document.getElementById(id);
  const e = UI.escape;
  let state = Hub.getState();
  let pendingKey = '', healthKey = '', sensorKey = '', proximityKey = '';
  let galleryRender = () => {}, galleryRefresh = () => {};
  let lastGallerySeq = -1;
  mountLangToggle('langBox');

  function renderRoute() {
    $('route').innerHTML = UI.route(page === 'visitor', page === 'admin');
    UI.highlight(state); UI.reveal();
  }
  function sharedRender(next) {
    state = next; UI.connection(state); UI.emergency(state);
    if (page !== 'entry') UI.robot(state); else UI.highlight(state);
    if (page === 'admin') renderDesk();
  }
  document.addEventListener('langchange', () => { renderRoute(); sharedRender(Hub.getState()); });

  if (page === 'entry') setupEntry();
  if (page === 'visitor') setupVisitor();
  if (page === 'admin') setupDesk();
  applyLang();
  Hub.subscribe(sharedRender);

  function setupEntry() {
    $('welcomeArt').innerHTML = UI.artwork('woodenboat');
    $('simBtn').hidden = Hub.isReal; $('previewHint').hidden = Hub.isReal;
    const valid = new Set(['MUSEUM-ENTRY-2026', 'http://192.168.4.1/api/enter']);
    let myId = null, busy = false, statusKey = '', statusClass = '', pollTimer;
    function status(key, kind = '') {
      statusKey = key; statusClass = kind;
      $('statusArea').textContent = t(key); $('statusArea').className = 'entry-status ' + kind;
    }
    function setBusy(on) {
      busy = on; $('qrfile').disabled = on; $('simBtn').disabled = on;
      document.querySelector('.camera-button').classList.toggle('is-disabled', on);
    }
    async function poll() {
      clearTimeout(pollTimer);
      try {
        const request = await Hub.getRequest(myId);
        if (request.status === 'approved') {
          status('gateApproved', 'is-success');
          setTimeout(() => { location.href = 'visitor.html?id=' + encodeURIComponent(myId); }, 700); return;
        }
        if (request.status === 'denied' || request.status === 'unknown') {
          status(request.status === 'denied' ? 'gateDenied' : 'requestUnknown', 'is-error'); setBusy(false); return;
        }
        status('gateWaiting', 'is-waiting');
      } catch (_) { status('requestError', 'is-error'); }
      pollTimer = setTimeout(poll, 1200);
    }
    async function begin() {
      if (busy) return;
      setBusy(true); status('gateWaiting', 'is-waiting');
      try { myId = await Hub.requestCheckIn(); poll(); }
      catch (_) { status('requestError', 'is-error'); setBusy(false); }
    }
    $('simBtn').addEventListener('click', begin);
    $('qrfile').addEventListener('change', event => {
      const file = event.target.files[0]; if (!file || busy) return;
      if (!Qr.available()) { status('gateScannerUnavailable', 'is-error'); event.target.value = ''; return; }
      setBusy(true); status('gateReading', 'is-waiting');
      Qr.decodeFile(file, (text, error) => {
        event.target.value = ''; setBusy(false);
        if (error || !text) { status('gateUnreadable', 'is-error'); return; }
        if (!valid.has(text.trim())) { status('gateInvalid', 'is-error'); return; }
        begin();
      });
    });
    document.addEventListener('langchange', () => { if (statusKey) status(statusKey, statusClass); });
    const autoId = new URLSearchParams(location.search).get('autoid');
    if (autoId) { myId = autoId; setBusy(true); status('gateWaiting', 'is-waiting'); poll(); }
  }
  function setupVisitor() {
    const id = new URLSearchParams(location.search).get('id');
    if (!id) return;
    let statusKey = '';
    const renderPass = () => { $('visitorIdNote').textContent = t('visitorRequest') + ' / ' + id + (statusKey ? ' · ' + t(statusKey) : ''); };
    renderPass();
    Hub.getRequest(id).then(request => { statusKey = ['approved', 'pending', 'denied'].includes(request.status) ? request.status : 'unknown'; renderPass(); })
      .catch(() => { $('visitorIdNote').textContent = t('requestError'); });
    document.addEventListener('langchange', renderPass);
  }

  function renderDesk() {
    $('approvedNum').textContent = state.approved;
    $('pendingNum').textContent = state.pending;
    $('muteBtn').textContent = t(state.muted ? 'muteOn' : 'muteOff');
    $('muteBtn').setAttribute('aria-pressed', String(!!state.muted));
    $('emergBtn').textContent = t(state.emergency ? 'emergencyOn' : 'emergencyOff');
    $('emergBtn').setAttribute('aria-pressed', String(!!state.emergency));
    $('emergBtn').classList.toggle('is-active', !!state.emergency);
    const requests = state.requests.filter(r => r.status === 'pending');
    const key = getLang() + JSON.stringify(requests);
    if (key !== pendingKey) {
      pendingKey = key;
      $('pending').innerHTML = requests.length ? requests.map(r => `<div class="pending-item"><div><span class="source-label">${t('requestId')}</span><strong class="request-id">${e(r.id)}</strong></div><div class="request-actions"><button class="button button--primary button--small" data-command data-action="approve" data-request="${e(r.id)}">${t('approve')}</button><button class="button button--secondary button--small" data-command data-action="deny" data-request="${e(r.id)}">${t('deny')}</button></div></div>`).join('') : `<div class="empty-requests"><span class="empty-mark" aria-hidden="true">✓</span><strong>${t('noPending')}</strong><p>${t('noPendingHint')}</p></div>`;
    }
    renderProximity(); renderHealth(); renderSensors();
    UI.connection(state);
    if (state.image.seq !== lastGallerySeq && state.connection.status !== 'disconnected' && state.connection.status !== 'connecting') galleryRefresh();
  }
  function renderProximity() {
    const available = state.connection.status === 'connected';
    const zones = [0, 2, 4].map(index => {
      const stops = ARTIFACTS.slice(index, index + 2);
      const rows = stops.map(stop => state.health.find(row => row.id === stop.id));
      if (!available) return 'unavailable';
      if (rows.some(row => row?.prox === true)) return 'proxDetected';
      if (rows.some(row => row?.proxRecent === true)) return 'proxRecent';
      return rows.some(row => row?.prox === false) ? 'proxClear' : 'unavailable';
    });
    const key = getLang() + zones.join(',');
    if (key === proximityKey) return;
    proximityKey = key;
    $('proximityZones').innerHTML = zones.map((status, index) => {
      const first = index * 2 + 1;
      return `<li data-status="${status}"><span>${t('proxStations')} ${first}–${first + 1}</span><strong>${t(status)}</strong></li>`;
    }).join('');
  }
  function channelSource(source) { return t(['measured', 'simulated', 'unavailable'].includes(source) ? source : 'unavailable'); }
  function reading(value, unit, source, realStation) {
    const available = UI.has(value) && !(realStation && source === 'simulated');
    return `<span class="reading">${available ? (source === 'estimated' ? '≈' : '') + e(Number(value.toFixed(1))) + (unit ? `<small>${unit}</small>` : '') : '—'}</span>${available ? '' : `<span class="reading-source">${t('unavailable')}</span>`}`;
  }
  function gasIndicator(status) {
    const labels = { normal: 'gasNormal', elevated: 'gasElevated', high: 'gasHigh', learning: 'gasLearning' };
    const key = Object.hasOwn(labels, status) ? status : 'unavailable';
    return `<span class="gas-status" data-level="${key}">${t(labels[key] || 'unavailable')}</span>`;
  }
  function renderHealth() {
    const key = getLang() + JSON.stringify(state.health) + String(state.profile?.gasCalibrated);
    if (key === healthKey) return; healthKey = key;
    $('healthBody').innerHTML = ARTIFACTS.map(a => {
      const h = state.health.find(row => row.id === a.id) || {};
      const thSource = h.thSource || 'unavailable';
      return `<tr class="${a.simulated ? 'health-demo' : 'health-real'}"><th scope="row"><div class="health-name"><span class="table-stop">0${a.station}</span><div>${e(a.name[getLang()])}</div></div></th>
        <td data-label="${e(t('thTemp'))}">${reading(h.temp, '°C', thSource, !a.simulated)}</td><td data-label="${e(t('thHum'))}">${reading(h.hum, '%', thSource, !a.simulated)}</td>
        <td data-label="${e(t('thGas'))}">${gasIndicator(h.gasStatus)}</td></tr>`;
    }).join('');
  }
  function renderSensors() {
    const key = getLang() + JSON.stringify(state.sensors) + state.health.map(row => row.gasStatus).join(','); if (key === sensorKey) return; sensorKey = key;
    $('sensorBody').innerHTML = state.sensors.length ? state.sensors.map(p => {
      const role = p.role || ([15, 16].includes(p.pin) ? 'dht11' : [4, 5].includes(p.pin) ? 'mq_ao' : '');
      const station = artifactById(p.station) || ARTIFACTS.find(a => a.station === Number(p.station));
      const readingText = role === 'mq_ao' ? gasIndicator(state.health.find(row => row.id === station?.id)?.gasStatus)
        : (UI.has(p.temp) ? e(p.temp) + ' °C' : '—') + ' / ' + (UI.has(p.hum) ? e(p.hum) + ' %' : '—');
      return `<tr><th scope="row">${e(p.pin)}<span class="reading-source">${e(role === 'dht11' ? 'DHT11' : role === 'mq_ao' ? 'MQ' : role)}</span></th><td>${station ? '0' + station.station + ' · ' + e(station.name[getLang()]) : '—'}</td><td class="utility">${readingText}</td><td class="utility">${role === 'dht11' ? e(p.ok ?? 0) + ' / ' + e(p.tries ?? 0) : '—'}</td><td>${channelSource(p.source === 'real' ? 'measured' : p.source)}<span class="reading-source">${role === 'dht11' ? e(p.dht || 'unprobed') : ''}</span></td></tr>`;
    }).join('') : `<tr><td colspan="5">${t('noSensors')}</td></tr>`;
  }
  function setupDesk() {
    let actionKey = '';
    function actionStatus(key, error) { actionKey = key; $('actionStatus').textContent = t(key); $('actionStatus').className = 'action-status' + (error ? ' is-error' : ''); }
    async function run(button, operation) {
      if (button.disabled) return;
      button.disabled = true; button.dataset.busy = 'true';
      try { await operation(); actionStatus('commandSent', false); }
      catch (_) { actionStatus('requestError', true); }
      finally { delete button.dataset.busy; UI.connection(Hub.getState()); }
    }
    $('pending').addEventListener('click', event => {
      const button = event.target.closest('[data-request]'); if (!button) return;
      run(button, () => button.dataset.action === 'approve' ? Hub.approve(button.dataset.request) : Hub.deny(button.dataset.request));
    });
    $('muteBtn').addEventListener('click', event => run(event.currentTarget, () => Hub.setMute(!state.muted)));
    $('emergBtn').addEventListener('click', event => run(event.currentTarget, () => Hub.setEmergency(!state.emergency)));
    $('advanceBtn').addEventListener('click', event => run(event.currentTarget, Hub.advanceRobot));
    document.addEventListener('langchange', () => { if (actionKey) $('actionStatus').textContent = t(actionKey); galleryRender(); });
    setupGallery(run);
  }
  function setupGallery(run) {
    let shots = [], busy = false, lastAttempt = 0, shownSeq = null, stripKey = '', selected = -1, galleryError = false, galleryMeta = {};
    const dialog = $('lightbox');
    function label(photo) {
      const artifact = artifactById(photo.station);
      let age = '';
      if (UI.has(photo.age)) age = photo.age < 0 ? t('cameraEarlierBoot') : (photo.age < 60000 ? Math.round(photo.age / 1000) + 's' : Math.round(photo.age / 60000) + 'm') + ' ' + t('cameraAgo');
      const demo = photo.index ? t('demoPhoto') + ' ' + photo.index + '/' + photo.total : '';
      return [demo, artifact?.name[getLang()], age].filter(Boolean).join(' · ');
    }
    function loadLatest(seq) {
      shownSeq = seq; let tries = 0;
      const img = $('latestImg'); $('imageError').hidden = true;
      img.onload = () => { $('imageError').hidden = true; };
      img.onerror = () => {
        if (shownSeq !== seq) return;
        if (tries >= 3) { $('imageError').hidden = false; return; }
        tries++;
        setTimeout(() => { if (shownSeq === seq) img.src = Hub.photoUrl(seq) + '&retry=' + tries; }, tries * 500);
      };
      img.src = Hub.photoUrl(seq);
    }
    galleryRender = function () {
      $('galleryEmpty').hidden = shots.length > 0;
      $('shotLatest').hidden = shots.length === 0;
      $('shotClear').hidden = shots.length === 0;
      $('galleryRetry').hidden = !galleryError;
      $('shotStore').textContent = shots.length ? `${galleryMeta.count ?? shots.length}/${galleryMeta.max ?? '—'} ${t('cameraStored')} · ${galleryMeta.simulated ? '' : (galleryMeta.backend || '')}` : '';
      $('imgHint').textContent = t(galleryError ? 'cameraError' : Hub.isReal ? 'cameraWaiting' : 'cameraPreview');
      $('shotStripWrap').hidden = shots.length < 2;
      if (!shots.length) return;
      const top = shots[0];
      $('latestImg').alt = label(top);
      if (shownSeq !== top.seq) loadLatest(top.seq);
      $('latestWhen').textContent = label(top); $('latestMeta').textContent = '#' + top.seq + ' · ' + top.bytes + ' B';
      $('latestOpen').setAttribute('aria-label', t('cameraTitle') + ' · ' + label(top));
      const key = getLang() + shots.slice(1).map(p => p.seq).join(',');
      if (key !== stripKey) {
        stripKey = key;
        $('shotStrip').innerHTML = shots.slice(1).map((p, i) => `<button class="shot-thumb" data-shot="${i + 1}" aria-label="${e(label(p) + ' · #' + p.seq)}"><img loading="lazy" src="${e(Hub.photoUrl(p.seq))}" alt="${e(label(p))}"><span>#${e(p.seq)}</span></button>`).join('');
      }
      if (dialog.open) updateViewer();
    };
    galleryRefresh = async function (force = false) {
      if (busy || (!force && Date.now() - lastAttempt < 3000)) return;
      busy = true; lastAttempt = Date.now();
      try {
        const gallery = await Hub.getGallery();
        galleryMeta = gallery;
        shots = Array.isArray(gallery.photos) ? gallery.photos : []; lastGallerySeq = gallery.newest ?? 0; galleryError = false;
        galleryRender();
      } catch (_) { galleryError = true; galleryRender(); }
      finally { busy = false; }
    };
    function updateViewer() {
      const photo = shots[selected]; if (!photo) { dialog.close(); return; }
      const url = Hub.photoUrl(photo.seq);
      if ($('lightboxImg').getAttribute('src') !== url) $('lightboxImg').src = url;
      $('lightboxCap').textContent = '#' + photo.seq + ' · ' + photo.bytes + ' B · ' + label(photo);
      $('shotOlder').disabled = selected >= shots.length - 1; $('shotNewer').disabled = selected <= 0;
    }
    function open(index) { selected = index; updateViewer(); if (!dialog.open) dialog.showModal(); }
    $('latestOpen').addEventListener('click', () => { if (!$('imageError').hidden && shots[0]) loadLatest(shots[0].seq); else open(0); });
    $('shotStrip').addEventListener('click', event => { const button = event.target.closest('[data-shot]'); if (button) open(Number(button.dataset.shot)); });
    $('shotClose').addEventListener('click', () => dialog.close());
    $('shotOlder').addEventListener('click', () => { if (selected < shots.length - 1) { selected++; updateViewer(); } });
    $('shotNewer').addEventListener('click', () => { if (selected > 0) { selected--; updateViewer(); } });
    dialog.addEventListener('click', event => { if (event.target === dialog && (event.clientY < dialog.getBoundingClientRect().top || event.clientY > dialog.getBoundingClientRect().bottom)) dialog.close(); });
    dialog.addEventListener('keydown', event => {
      if (event.key === 'ArrowLeft' && selected < shots.length - 1) { selected++; updateViewer(); event.preventDefault(); }
      if (event.key === 'ArrowRight' && selected > 0) { selected--; updateViewer(); event.preventDefault(); }
    });
    $('shotClear').addEventListener('click', event => {
      if (!confirm(t('cameraClearAsk'))) return;
      run(event.currentTarget, async () => { await Hub.clearGallery(); shownSeq = null; stripKey = ''; await galleryRefresh(true); });
    });
    $('galleryRetry').addEventListener('click', () => galleryRefresh(true));
    galleryRender();
  }
})();
