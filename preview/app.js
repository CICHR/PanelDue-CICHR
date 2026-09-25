(() => {
  const cfg = structuredClone(window.PANELDUE_PREVIEW_CONFIG);
  const screen = document.getElementById('screen');
  let page = 'home';
  let adjustment = null;

  const icons = {
    home:'<path d="M3 11.5 12 4l9 7.5"/><path d="M5.5 10.5V21h13V10.5"/><path d="M9.5 21v-6h5v6"/>',
    print:'<path d="M7 3h10v5H7z"/><path d="M6 17H4a2 2 0 0 1-2-2v-4a3 3 0 0 1 3-3h14a3 3 0 0 1 3 3v4a2 2 0 0 1-2 2h-2"/><path d="M6 14h12v7H6z"/>',
    console:'<path d="m4 6 5 5-5 5"/><path d="M11 18h9"/>',
    settings:'<circle cx="12" cy="12" r="3"/><path d="M19 12a7 7 0 0 0-.1-1l2-1.5-2-3.4-2.5 1a7 7 0 0 0-1.7-1L14.3 3h-4.6L9.3 6.1a7 7 0 0 0-1.7 1l-2.5-1-2 3.4 2 1.5a7 7 0 0 0 0 2l-2 1.5 2 3.4 2.5-1a7 7 0 0 0 1.7 1l.4 3.1h4.6l.4-3.1a7 7 0 0 0 1.7-1l2.5 1 2-3.4-2-1.5c.1-.3.1-.7.1-1z"/>',
    stop:'<rect x="5" y="5" width="14" height="14" rx="1"/>',
    close:'<path d="M6 6l12 12M18 6 6 18"/>',
    bed:'<rect x="4" y="7" width="16" height="10" rx="1"/><path d="M7 4v3M12 4v3M17 4v3"/>',
    nozzle:'<path d="M7 4h10l-1 6-4 4-4-4z"/><path d="M10 14h4l-2 6z"/>',
    fan:'<circle cx="12" cy="12" r="8"/><circle cx="12" cy="12" r="1.3"/><path d="M13 10c1-4 4-5 5-3s-1 5-5 6M14 13c4 1 5 4 3 5s-5-1-6-5M11 14c-1 4-4 5-5 3s1-5 5-6M10 11C6 10 5 7 7 6s5 1 6 5"/>',
    output:'<rect x="4" y="4" width="16" height="16" rx="2"/><path d="m13 5-5 8h4l-1 6 5-8h-4z"/>',
    plus:'<path d="M12 4v16M4 12h16"/>',
    files:'<path d="M3 7h7l2 2h9v10H3z"/>',
    move:'<path d="M12 3v18M3 12h18"/><path d="m8 7 4-4 4 4M17 8l4 4-4 4M8 17l4 4 4-4M7 8l-4 4 4 4"/>',
    extrude:'<path d="M7 3h10v6H7z"/><path d="M8 9h8l-2 5h-4z"/><path d="M10 14h4l-2 7z"/>',
    macro:'<rect x="4" y="5" width="16" height="14" rx="1"/><path d="m7 9 3 3-3 3M12 15h5"/>',
    pause:'<path d="M8 5v14M16 5v14"/>',
    play:'<path d="m8 5 11 7-11 7z"/>',
    left:'<path d="m15 5-7 7 7 7"/>', right:'<path d="m9 5 7 7-7 7"/>', up:'<path d="m5 15 7-7 7 7"/>', down:'<path d="m5 9 7 7 7-7"/>'
  };
  const svg = n => `<svg viewBox="0 0 24 24">${icons[n] || ''}</svg>`;
  const esc = s => String(s ?? '').replace(/[&<>"']/g, c => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));

  function shellHtml() {
    return `<div class="shell">
      <div class="rail">
        ${nav('home','home','Home','nav-home')}
        ${nav('print','print','Print','nav-print')}
        ${nav('console','console','Console','nav-console')}
        ${nav('settings','settings','Settings','nav-settings')}
      </div>
      <div class="topbar"></div>
      <div class="machine-name" id="machineName"></div>
      <button class="stop" id="globalStop">${svg('stop')}<span>STOP</span></button>
      <div class="machine-status" id="machineStatus"></div>
      <div class="page" id="page-home"></div>
      <div class="page" id="page-print"></div>
      <div class="page" id="page-console"></div>
      <div class="page" id="page-settings"></div>
      <div class="motion" id="motion"></div>
      <div class="dimmer" id="dimmer"></div>
      <div class="adjust-popup" id="adjustPopup"></div>
      <div class="slot-popup" id="slotPopup"></div>
    </div>`;
  }
  function nav(p, i, label, cls) { return `<button class="nav ${cls}" data-page="${p}">${svg(i)}<span>${label}</span></button>`; }

  function pageBase() {
    document.getElementById('machineName').textContent = page === 'print' ? cfg.printFile : page === 'console' ? 'Console' : page === 'settings' ? 'Settings' : cfg.machineName;
    document.getElementById('machineStatus').textContent = (page === 'home' || page === 'print') ? cfg.status : '';
  }

  function renderHome() {
    const el = document.getElementById('page-home');
    let h = '';
    for (let i=0;i<12;i++) {
      const x = 102 + (i%4)*172;
      const y = 64 + Math.floor(i/4)*92;
      const t = cfg.temperatures[i];
      h += `<div class="panel home-card" style="left:${x}px;top:${y}px">`;
      if (t) {
        h += `<div class="slot-title">${svg(t.icon || (t.type==='bed'?'bed':'nozzle'))}${t.type==='bed' ? '' : `<span>${esc(t.index ?? i)}</span>`}</div>`;
        h += `<div class="live-temp">${Number(t.current).toFixed(1)}°C</div>`;
        h += `<button class="setpoint active-temp" data-adjust="temp-active" data-index="${i}">A ${Math.round(t.active)}°</button>`;
        h += `<button class="setpoint standby-temp" data-adjust="temp-standby" data-index="${i}">S ${Math.round(t.standby)}°</button>`;
      } else {
        const aux = cfg.homeSlots[i-cfg.temperatures.length] || {type:'empty'};
        if (aux.type === 'fan' && cfg.fans[aux.index]) {
          const f=cfg.fans[aux.index];
          h += `<div class="aux-name">${esc(f.name || `F${aux.index}`)}</div><button class="aux-value" data-adjust="fan" data-index="${aux.index}">${svg('fan')}<span>${Math.round(f.value)}%</span></button>`;
        } else if (aux.type === 'output' && cfg.outputs[aux.index]) {
          const o=cfg.outputs[aux.index];
          h += `<div class="aux-name">${esc(o.name || `Output ${aux.index}`)}</div><button class="aux-value" data-adjust="output" data-index="${aux.index}">${svg('output')}<span>${Math.round(o.value)}%</span></button>`;
        } else {
          h += `<button class="plus" data-slot="${i}">${svg('plus')}</button>`;
        }
      }
      h += `</div>`;
    }
    const homed = cfg.homed || {x:true,y:true,z:true};
    const notHomed = ['X','Y','Z'].filter(a => !homed[a.toLowerCase()]);
    const machineTitle = notHomed.length ? 'MACHINE' : 'MACHINE POSITION';
    const homedText = notHomed.length ? `NOT HOMED: ${notHomed.join(' ')}` : 'ALL AXES HOMED';
    h += `<button class="hw-button home-axis-button homeall">${svg('home')}</button>`;
    h += `<button class="hw-button home-axis-button noicon homex">X</button><button class="hw-button home-axis-button noicon homey">Y</button><button class="hw-button home-axis-button noicon homez">Z</button>`;
    h += `<div class="panel machine-panel"><div class="caption">${machineTitle}</div><div class="not-homed">${homedText}</div><div class="axis x">X${cfg.position.x.toFixed(1)} mm</div><div class="axis y">Y${cfg.position.y.toFixed(1)} mm</div><div class="axis z">Z${cfg.position.z.toFixed(1)} mm</div></div>`;
    h += `<button class="hw-button home-action act-files">${svg('files')}<span>Files</span></button>`;
    h += `<button class="hw-button home-action act-move" id="openMotion">${svg('move')}<span>Move</span></button>`;
    h += `<button class="hw-button home-action act-extrude">${svg('extrude')}<span>Extrude</span></button>`;
    h += `<button class="hw-button home-action act-macros">${svg('macro')}<span>Macros</span></button>`;
    el.innerHTML = h;
  }

  function renderPrint() {
    const el = document.getElementById('page-print');
    let h='';
    for (let i=0;i<cfg.temperatures.length && i<8;i++) {
      const t=cfg.temperatures[i], x=102+i*85;
      h += `<div class="panel print-temp" style="left:${x}px"><div class="pt-title">${svg(t.icon || (t.type==='bed'?'bed':'nozzle'))}${t.type==='bed' ? '' : `<span>${esc(t.index ?? i)}</span>`}</div><div class="pt-value">${Number(t.current).toFixed(1)}°C</div></div>`;
    }
    h += `<div class="panel job-panel"><div class="job-title">JOB / POSITION</div><div class="job-axis job-x">X${cfg.position.x.toFixed(1)} mm</div><div class="job-axis job-y">Y${cfg.position.y.toFixed(1)} mm</div><div class="job-axis job-z">Z${cfg.position.z.toFixed(1)} mm</div><div class="job-time">Time ${esc(cfg.elapsed)}</div><div class="job-eta">ETA ${esc(cfg.eta)}</div><div class="job-progress">Progress ${Math.round(cfg.progress)}%</div><div class="progress-track"><i style="width:${Math.max(0,Math.min(100,cfg.progress))}%"></i></div></div>`;
    h += `<button class="hw-button print-action pa1">${svg('move')}Baby Z</button><button class="hw-button print-action pa2">${svg('pause')}Pause</button><button class="hw-button print-action pa3">${svg('stop')}Cancel</button><button class="hw-button print-action pa4">${svg('print')}Reprint</button><button class="hw-button print-action pa5" data-adjust="speed">Speed ${Math.round(cfg.speed)}%</button>`;
    h += `<div class="io-heading fans-title">FANS</div><div class="io-heading outs-title">OUTPUTS</div>`;
    cfg.fans.slice(0,8).forEach((f,i)=>{ const col=i&3,rowTop=i<4?262:304,x=102+col*172; h += `<div class="io-name" style="left:${x+4}px;top:${rowTop}px">${esc(f.name || `F${i}`)}</div><button class="io-button" style="left:${x}px;top:${rowTop+19}px" data-adjust="fan" data-index="${i}">${svg('fan')}<span>${Math.round(f.value)}%</span></button>`; });
    cfg.outputs.slice(0,8).forEach((o,i)=>{ const col=i&3,rowTop=i<4?369:411,x=102+col*172; h += `<div class="io-name" style="left:${x+4}px;top:${rowTop}px">${esc(o.name || `Output ${i}`)}</div><button class="io-button" style="left:${x}px;top:${rowTop+19}px" data-adjust="output" data-index="${i}">${svg('output')}<span>${Math.round(o.value)}%</span></button>`; });
    el.innerHTML=h;
  }

  function renderConsole() {
    const el=document.getElementById('page-console');
    let h=`<div class="console-title">Console</div>`;
    const lines=[['00:00','PanelDue CICHR connected'],['00:01','RepRapFirmware object model ready'],['00:03','ok'],['00:07','M115'],['00:07','FIRMWARE_NAME: RepRapFirmware'],['00:08','ok'],['00:12','']];
    lines.forEach((l,i)=>{ const y=60+i*21; h+=`<div class="console-line" style="top:${y}px"><span class="time">${l[0]}</span>${esc(l[1])}</div>`; });
    h+=`<div class="console-input" id="consoleInput">_</div>`;
    const rows=['QWERTYUIOP[]','ASDFGHJKL;\'','ZXCVBNM,./','1234567890-'];
    rows.forEach((row,ri)=>{ [...row].forEach((ch,ci)=>{ if(ci>11)return; h+=`<button class="key" data-key="${esc(ch)}" style="left:${106+ci*50}px;top:${279+ri*41}px;width:46px">${esc(ch)}</button>`; }); });
    h+=`<button class="key" data-key="BACK" style="left:722px;top:279px;width:58px">⌫</button>`;
    h+=`<button class="key" data-key="UP" style="left:712px;top:361px;width:68px">↑</button><button class="key" data-key="DOWN" style="left:712px;top:402px;width:68px">↓</button>`;
    h+=`<button class="key" data-key="SHIFT" style="left:106px;top:443px;width:131px">Shift</button><button class="key" data-key="SPACE" style="left:241px;top:443px;width:262px"></button><button class="key" data-key="ENTER" style="left:507px;top:443px;width:131px">↵</button>`;
    el.innerHTML=h;
  }

  function renderSettings() {
    const s=cfg.settings, el=document.getElementById('page-settings');
    const rows=[
      [s.language,`Baud  ${s.baud}`],
      [`Volume  ${s.volume}`,s.colour],
      ['Brightness -','Brightness +'],
      [s.dimming,`Info timeout  ${s.infoTimeout}`],
      [`Screensaver  ${s.screensaver}`,`Babystep ${s.babystep}`],
      [`Feedrate  ${s.feedrate}`,s.heaterCombine],
      [s.logLevel,'Calibrate touch'],
      ['Mirror display','Invert display']
    ];
    let h=`<div class="settings-title">Settings</div><div class="settings-info fwinfo">FW - PanelDue-CICHR v1.0.0 [v3-5.0]</div><div class="settings-info ipinfo">IP ${esc(s.ip)}</div>`;
    rows.forEach((r,i)=>{const y=82+i*39;h+=`<button class="settings-btn left" style="top:${y}px">${esc(r[0])}</button><button class="settings-btn right" style="top:${y}px">${esc(r[1])}</button>`;});
    h+=`<button class="settings-btn left" data-invert-z style="top:${82+8*39}px">Invert Z: ${s.invertZ?'ON':'OFF'}</button>`;
    h+=`<button class="settings-btn factory" style="top:${82+9*39}px">Clear settings</button>`;
    el.innerHTML=h;
  }

  function renderMotion() {
    const m=document.getElementById('motion');
    let h=`<button class="stop motion-stop" id="motionStop">${svg('stop')}<span>STOP</span></button><div class="motion-title">MOTION</div><button class="motion-close" id="motionClose">${svg('close')}</button>`;
    h+=`<div class="panel work-panel"></div><div class="motion-work-label">WORK AREA</div><div class="bedbox"><div class="bedmap" id="bedmap"><div class="bedgrid"></div><div class="tool-dot" id="toolDot"></div></div></div><div class="motion-pos mpx">X${cfg.position.x.toFixed(1)} mm</div><div class="motion-pos mpy">Y${cfg.position.y.toFixed(1)} mm</div><div class="motion-pos mpz">Z${cfg.position.z.toFixed(1)} mm</div>`;
    h+=`<div class="panel jog-panel"></div><div class="jog-label">JOG</div><div class="zlabel">Z</div><button class="jog-btn jyplus" data-jog="Y" data-sign="1">${svg('up')}+Y</button><button class="jog-btn jxminus" data-jog="X" data-sign="-1">${svg('left')}-X</button><button class="jog-btn jxplus" data-jog="X" data-sign="1">${svg('right')}+X</button><button class="jog-btn jyminus" data-jog="Y" data-sign="-1">${svg('down')}-Y</button><button class="jog-btn jzplus" data-jog="Z" data-sign="${cfg.settings.invertZ?-1:1}">${svg('up')}${cfg.settings.invertZ?'-Z':'Z+'}</button><button class="jog-btn jzminus" data-jog="Z" data-sign="${cfg.settings.invertZ?1:-1}">${svg('down')}${cfg.settings.invertZ?'Z+':'-Z'}</button>`;
    h+=`<div class="panel step-panel"></div><div class="step-label">STEP  mm</div><div class="feed-label">SPEED F  mm/s</div>`;
    ['0.1','1','10','100'].forEach((v,i)=>h+=`<button class="seg step" data-v="${v}" style="left:${442+i*76}px;top:274px;width:70px">${v}</button>`);
    ['1','5','10','25','50'].forEach((v,i)=>h+=`<button class="seg feed${v==='25'?' active':''}" data-v="${v}" style="left:${442+i*64}px;top:340px;width:58px">${v}</button>`);
    h+=`<div class="panel extrude-panel"></div><div class="tool-chip">T0</div><button class="extr-btn retract">${svg('extrude')}RETRACT</button><button class="extr-btn extrude">${svg('extrude')}EXTRUDE</button>`;
    m.innerHTML=h;
    updateBedMap();
  }

  function updateBedMap(){
    const b=document.getElementById('bedmap'),d=document.getElementById('toolDot'); if(!b||!d)return;
    const bx=cfg.bed.xMax-cfg.bed.xMin, by=cfg.bed.yMax-cfg.bed.yMin; let w=378,h=310;
    if(cfg.bed.circular){w=h=Math.min(w,h);b.classList.add('circle')}else{const scale=Math.min(378/bx,310/by);w=Math.max(80,Math.round(bx*scale));h=Math.max(80,Math.round(by*scale));b.classList.remove('circle')}
    b.style.width=w+'px';b.style.height=h+'px';b.style.left=Math.floor((378-w)/2)+'px';b.style.top=Math.floor((310-h)/2)+'px';
    const nx=(cfg.position.x-cfg.bed.xMin)/bx,ny=(cfg.position.y-cfg.bed.yMin)/by;d.style.left=(Math.max(0,Math.min(1,nx))*100)+'%';d.style.top=((1-Math.max(0,Math.min(1,ny)))*100)+'%';
  }

  function adjustDescriptor(kind,index){
    if(kind==='fan'){const f=cfg.fans[index];return {name:f?.name||`F${index}`,icon:'fan',unit:'%',get:()=>f.value,set:v=>f.value=v,min:0,max:100};}
    if(kind==='output'){const o=cfg.outputs[index];return {name:o?.name||`Output ${index}`,icon:'output',unit:'%',get:()=>o.value,set:v=>o.value=v,min:0,max:100};}
    if(kind==='speed'){return {name:'Print speed',icon:'print',unit:'%',get:()=>cfg.speed,set:v=>cfg.speed=v,min:1,max:500};}
    const t=cfg.temperatures[index];
    if(kind==='temp-active')return {name:`${t.label} - active`,icon:t.icon||'nozzle',unit:'°C',get:()=>t.active,set:v=>t.active=v,min:0,max:500};
    return {name:`${t.label} - standby`,icon:t.icon||'nozzle',unit:'°C',get:()=>t.standby,set:v=>t.standby=v,min:0,max:500};
  }

  function openAdjust(kind,index=0){
    adjustment=adjustDescriptor(kind,index); renderAdjust(); document.getElementById('dimmer').classList.add('active'); document.getElementById('adjustPopup').classList.add('active');
  }
  function renderAdjust(){
    const p=document.getElementById('adjustPopup'),v=Math.round(adjustment.get());
    const widths=[102,102,102,102,102], xs=[20,134,248,362,476];
    p.innerHTML=`<div class="adjust-icon">${svg(adjustment.icon)}</div><div class="adjust-name">${esc(adjustment.name)}</div><div class="adjust-value">${v} ${adjustment.unit}</div>`+
      [-5,-1,0,1,5].map((n,i)=>`<button class="adjust-step" style="left:${xs[i]}px;width:${widths[i]}px" data-step="${n}">${n===0?'Set':n>0?'+'+n:n}</button>`).join('');
  }
  function closeAdjust(){document.getElementById('dimmer').classList.remove('active');document.getElementById('adjustPopup').classList.remove('active');adjustment=null;renderHome();renderPrint();showPage(page);}

  function openSlot(slot){
    const p=document.getElementById('slotPopup'); let h=`<div class="slot-popup-title">HOME SLOT</div><div class="slot-grid">`;
    for(let i=0;i<8;i++){const f=cfg.fans[i];h+=`<button data-slot-kind="fan" data-index="${i}" data-slot="${slot}">${esc(f?.name||`F${i}`)}</button>`;}
    for(let i=0;i<8;i++){const o=cfg.outputs[i];h+=`<button data-slot-kind="output" data-index="${i}" data-slot="${slot}">${esc(o?.name||`Output ${i}`)}</button>`;}
    h+=`</div>`;p.innerHTML=h;document.getElementById('dimmer').classList.add('active');p.classList.add('active');
  }
  function closeSlot(){document.getElementById('dimmer').classList.remove('active');document.getElementById('slotPopup').classList.remove('active');}

  function showPage(p){
    page=p; document.querySelectorAll('.page').forEach(x=>x.classList.toggle('active',x.id===`page-${p}`));document.querySelectorAll('.nav').forEach(x=>x.classList.toggle('active',x.dataset.page===p));pageBase();
  }

  function bind(){
    screen.addEventListener('click',e=>{
      const nav=e.target.closest('[data-page]'); if(nav){showPage(nav.dataset.page);return;}
      if(e.target.closest('#openMotion')){document.getElementById('motion').classList.add('active');return;}
      if(e.target.closest('#motionClose')){document.getElementById('motion').classList.remove('active');return;}
      const a=e.target.closest('[data-adjust]'); if(a){openAdjust(a.dataset.adjust,Number(a.dataset.index||0));return;}
      const st=e.target.closest('.adjust-step'); if(st&&adjustment){const n=Number(st.dataset.step);if(n===0){closeAdjust();return;}adjustment.set(Math.max(adjustment.min,Math.min(adjustment.max,Math.round(adjustment.get())+n)));renderAdjust();renderHome();renderPrint();return;}
      const slot=e.target.closest('[data-slot]'); if(slot && slot.classList.contains('plus')){openSlot(Number(slot.dataset.slot));return;}
      const pick=e.target.closest('[data-slot-kind]'); if(pick){const slotIndex=Number(pick.dataset.slot),auxIndex=slotIndex-cfg.temperatures.length;if(auxIndex>=0)cfg.homeSlots[auxIndex]={type:pick.dataset.slotKind,index:Number(pick.dataset.index)};closeSlot();renderHome();showPage(page);return;}
      const invertZ=e.target.closest('[data-invert-z]'); if(invertZ){cfg.settings.invertZ=!cfg.settings.invertZ;renderSettings();renderMotion();return;}
      const jog=e.target.closest('[data-jog]'); if(jog){const axis=jog.dataset.jog.toLowerCase(),step=10*Number(jog.dataset.sign);cfg.position[axis]+=step;renderMotion();return;}
      const key=e.target.closest('[data-key]'); if(key){const inp=document.getElementById('consoleInput');let t=inp.textContent==='_'?'':inp.textContent;if(key.dataset.key==='BACK')t=t.slice(0,-1);else if(key.dataset.key==='SPACE')t+=' ';else if(!['UP','DOWN','SHIFT','ENTER'].includes(key.dataset.key))t+=key.dataset.key;inp.textContent=t+'_';return;}
    });
    document.getElementById('dimmer').addEventListener('click',()=>{closeSlot();if(adjustment)closeAdjust();});
  }

  screen.innerHTML=shellHtml();
  renderHome();renderPrint();renderConsole();renderSettings();renderMotion();bind();showPage('home');
})();
