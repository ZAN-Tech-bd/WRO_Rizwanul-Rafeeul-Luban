/* Only this adapter talks to the Hub. Other hosts are explicitly local previews. */
const Hub = (() => {
  const REAL = location.hostname === '192.168.4.1';
  const prefix = 'novhub:preview:v2:';
  const read = (key, fallback) => { try { return JSON.parse(localStorage.getItem(prefix + key)) ?? fallback; } catch (_) { return fallback; } };
  function write(key, value) {
    localStorage.setItem(prefix + key, JSON.stringify(value));
    window.dispatchEvent(new Event('novhub:change'));
  }
  const profile = { name: 'NOV_HUB', stationCount: 6, realStations: 6, simulatedStations: 0, gasCalibrated: false };
  const empty = { profile, robot: { station: '', status: '' }, emergency: false, muted: false, approved: 0, pending: 0, health: [], sensors: [], requests: [], image: { bytes: 0, station: '', seq: 0 } };
  let cache = { ...empty, connection: { mode: REAL ? 'hub' : 'preview', status: REAL ? 'connecting' : 'preview', lastSeen: null } };
  const subscribers = new Set();
  let started = false, polling = false;
  function publish(state) { cache = state; subscribers.forEach(cb => { try { cb(cache); } catch (error) { console.error('NOV_HUB render:', error); } }); }
  async function request(url, method = 'GET') {
    const controller = new AbortController();
    const timeout = setTimeout(() => controller.abort(), 5000);
    try {
      const response = await fetch(url, { method, signal: controller.signal, cache: 'no-store' });
      if (!response.ok) throw new Error('HTTP ' + response.status);
      const text = await response.text();
      if (!text) return {};
      try { return JSON.parse(text); } catch (_) { if (method === 'GET') throw new Error('Invalid JSON'); return {}; }
    } finally { clearTimeout(timeout); }
  }
  function disconnected(error) { publish({ ...cache, connection: { ...cache.connection, status: 'disconnected', error: error.message } }); }
  async function pollReal() {
    if (polling) return;
    polling = true;
    try {
      const state = await request('/api/state');
      if (!state || typeof state !== 'object' || !state.robot) throw new Error('Invalid Hub state');
      publish({ ...empty, ...state, requests: state.requests || [], sensors: state.sensors || [], health: state.health || [], connection: { mode: 'hub', status: 'connected', lastSeen: Date.now() } });
    } catch (error) { disconnected(error); }
    finally { polling = false; }
  }
  function mockSensors() { return []; }
  function mockState() {
    const requests = read('requests', []);
    return { ...empty, requests, approved: requests.filter(r => r.status === 'approved').length, pending: requests.filter(r => r.status === 'pending').length,
      robot: read('robot', { station: 'terracotta', status: 'narrating' }), emergency: read('emergency', false), muted: read('muted', false),
      health: ARTIFACTS.map((a, i) => ({ id: a.id, temp: a.simulated ? 27 + i * .3 : null, hum: a.simulated ? 57 + i : null, gas: a.simulated ? 1800 + i * 63 : null, gasOk: null,
        derived: a.simulated, simulated: a.simulated, source: a.simulated ? 'simulated' : 'real', thSource: a.simulated ? 'simulated' : 'unavailable', gasSource: a.simulated ? 'simulated' : 'unavailable', prox: a.simulated ? false : null, proxShared: !a.simulated })),
      sensors: mockSensors(), connection: { mode: 'preview', status: 'preview', lastSeen: null }
    };
  }
  function pollMock() { publish(mockState()); }
  function subscribe(callback) {
    subscribers.add(callback); callback(cache);
    if (!started) {
      started = true;
      if (REAL) { pollReal(); setInterval(pollReal, 1500); }
      else { pollMock(); window.addEventListener('storage', pollMock); window.addEventListener('novhub:change', pollMock); }
    }
    return () => subscribers.delete(callback);
  }
  async function command(url) {
    try { await request(url, 'POST'); await pollReal(); } catch (error) { disconnected(error); throw error; }
  }
  async function requestCheckIn() {
    if (REAL) {
      const result = await request('/api/checkin', 'POST');
      if (!result.id) throw new Error('Missing request ID');
      return result.id;
    }
    const requests = read('requests', []), id = 'V' + Date.now().toString(36) + Math.random().toString(36).slice(2, 5);
    requests.push({ id, ts: Date.now(), status: 'pending' }); write('requests', requests); return id;
  }
  async function getRequest(id) {
    if (REAL) return request('/api/request?id=' + encodeURIComponent(id));
    return read('requests', []).find(r => r.id === id) || { id, status: 'unknown' };
  }
  async function decide(id, status) {
    if (REAL) return command('/api/' + (status === 'approved' ? 'approve' : 'deny') + '?id=' + encodeURIComponent(id));
    const requests = read('requests', []), item = requests.find(r => r.id === id);
    if (item) item.status = status;
    write('requests', requests);
  }
  async function setEmergency(on) { if (REAL) return command('/api/emergency?on=' + (on ? 1 : 0)); write('emergency', !!on); }
  async function setMute(on) { if (REAL) return command('/api/audio/mute?on=' + (on ? 1 : 0)); write('muted', !!on); }
  async function advanceRobot() {
    if (REAL) return command('/api/robot/advance');
    const index = ARTIFACTS.findIndex(a => a.id === read('robot', { station: 'terracotta' }).station);
    const next = ARTIFACTS[(index + 1) % ARTIFACTS.length];
    write('robot', { station: next.id, status: 'moving' });
    setTimeout(() => write('robot', { station: next.id, status: 'narrating' }), 1500);
  }
  async function getGallery() { return REAL ? request('/api/gallery') : { backend: 'preview', count: 0, max: 0, newest: 0, photos: [] }; }
  async function clearGallery() { if (REAL) return command('/api/gallery/clear'); }
  return { isReal: REAL, subscribe, getState: () => cache, retry: () => REAL ? pollReal() : pollMock(),
    listPending: () => cache.requests.filter(r => r.status === 'pending'), countApproved: () => cache.approved, countPending: () => cache.pending,
    requestCheckIn, getRequest, approve: id => decide(id, 'approved'), deny: id => decide(id, 'denied'), setEmergency, setMute, advanceRobot, getGallery, clearGallery,
    photoUrl: seq => '/api/image?seq=' + encodeURIComponent(seq) };
})();
