// WRITES ONLY J3's saved records; undo the partial move after the rejected pad edit.
const BASE = __BASE_RECORDS__;
const p = await eda.dmt_Project.getCurrentProjectInfo();
const doc = await eda.dmt_SelectControl.getCurrentDocumentInfo();
if (p?.friendlyName !== 'WRO_HUB' || doc?.uuid !== 'ad35aed4d7610fd2') throw Error('Wrong project/document');
const parse = s => s.trim().split(/\r?\n/).map(l => {
  const i = l.indexOf('||');
  return [JSON.parse(l.slice(0, i)), JSON.parse(l.slice(i + 2).replace(/\|\s*$/, ''))];
});
const live = parse(await eda.sys_FileManager.getDocumentSource());
const originals = new Map(BASE.map(r => [r[0].id, r]));
const target='0cbe9290d583d889';
if(!live.some(([a])=>a.type==='COMPONENT'&&a.id===target)) throw Error('J3 component identity changed; refusing restore');
// An invalid footprint save may delete its pad-net records. Restore the complete saved set,
// including missing pad-net rows; never touch any unrelated component or copper record.
const out=live.filter(([a,v])=>!originals.has(a.id) && v.parentId!==target &&
  !(a.type==='PAD_NET' && String(a.id).includes(target)));
out.push(...BASE);
const changed=BASE.length;
if (!await eda.sys_FileManager.setDocumentSource(out.map(r => JSON.stringify(r[0]) + '||' + JSON.stringify(r[1]) + '|').join('\n') + '\n'))
  throw Error('Restore rejected');
await eda.pcb_Document.save();
const c = (await eda.pcb_PrimitiveComponent.get(await eda.pcb_PrimitiveComponent.getAllPrimitiveId())).find(c => c.getState_Designator() === 'J3');
return { restoredJ3Records: changed, x: c.getState_X(), y: c.getState_Y(), mpn: c.getState_ManufacturerId(), supplierId: c.getState_SupplierId(), pads: (await c.getAllPins()).length };
