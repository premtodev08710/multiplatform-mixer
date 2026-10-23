/**
 * Mix Practice Studio Lite — Complete JUCE 8 UI & Web Audio DSP Engine Replica
 * - Locked 100% Viewport Scale (Zero vertical/horizontal page scrolling on iOS/Android/Desktop)
 * - Exact visual & functional match to Source/UI/* (Home, Detail, Tracks, Effects, Scenes, Setup)
 * - Practice Lite Limits: 16 Input Tracks Max, 4 Studio FX Slots Max
 */

(function () {
  'use strict';

  const kMaxTracks = 16;
  const kMaxFxSlots = 4;
  const kFaderMinDb = -90.0;
  const kFaderMaxDb = 10.0;

  // --- Console Fader Taper (Matches DspMath.h faderDbToPos / faderPosToDb) ---
  function faderDbToPos(db) {
    if (db <= -90.0) return 0.0;
    if (db >= 10.0) return 1.0;
    if (db >= -10.0) return 0.5 + (db + 10.0) / 40.0; // -10..+10 maps to 0.5..1.0 (0dB = 0.75)
    if (db >= -30.0) return 0.25 + (db + 30.0) / 80.0; // -30..-10 maps to 0.25..0.5
    if (db >= -60.0) return 0.08 + (db + 60.0) / 176.47;
    return Math.max(0.0, ((db + 90.0) / 30.0) * 0.08);
  }

  function faderPosToDb(pos) {
    const p = Math.max(0.0, Math.min(1.0, pos));
    if (p <= 0.001) return -90.0;
    if (p >= 0.5) return -10.0 + (p - 0.5) * 40.0;
    if (p >= 0.25) return -30.0 + (p - 0.25) * 80.0;
    if (p >= 0.08) return -60.0 + ((p - 0.08) / 0.17) * 30.0;
    return -90.0 + (p / 0.08) * 30.0;
  }

  function dbToGain(db) {
    if (db <= -89.5) return 0.0;
    return Math.pow(10.0, db / 20.0);
  }

  function gainToDb(g) {
    if (g <= 0.00003) return -90.0;
    return Math.max(-90.0, 20.0 * Math.log10(g));
  }

  function formatDbText(db) {
    if (db <= -89.0) return '-inf';
    return db.toFixed(1) + ' dB';
  }

  function formatTimeSec(sec) {
    if (!sec || sec <= 0) return '00:00.00';
    const m = Math.floor(sec / 60);
    const s = sec % 60;
    return String(m).padStart(2, '0') + ':' + s.toFixed(2).padStart(5, '0');
  }

  // --- Channel Color Palette (Matches DetailScreen.cpp palette) ---
  const kChannelPalette = [
    '#ffc107', '#ff9800', '#e91e63', '#2196f3',
    '#4caf50', '#00bcd4', '#9c27b0', '#795548'
  ];

  // --- FX Catalog (4 Active Slots for Practice Lite) ---
  const kFxCatalog = [
    {
      id: 'hall', name: 'Hall Reverb', shortName: 'HALL', family: 'larc',
      params: [
        { name: 'PRE DEL', min: 0, max: 100, val: 24, unit: 'ms' },
        { name: 'DECAY', min: 0.4, max: 6.0, val: 3.2, unit: 's' },
        { name: 'SIZE', min: 10, max: 100, val: 72, unit: '%' },
        { name: 'DAMPING', min: 1, max: 16, val: 7, unit: 'kHz' },
        { name: 'DIFFUSE', min: 0.5, max: 2.0, val: 1.62, unit: '' },
        { name: 'LEVEL', min: -30, max: 6, val: 0, unit: 'dB' }
      ]
    },
    {
      id: 'plate', name: 'Rich Plate Reverb', shortName: 'PLATE', family: 'larc',
      params: [
        { name: 'PRE DEL', min: 0, max: 100, val: 12, unit: 'ms' },
        { name: 'DECAY', min: 0.3, max: 4.5, val: 1.8, unit: 's' },
        { name: 'SIZE', min: 10, max: 100, val: 65, unit: '%' },
        { name: 'DAMPING', min: 1, max: 16, val: 9.5, unit: 'kHz' },
        { name: 'DIFFUSE', min: 0.5, max: 2.0, val: 1.80, unit: '' },
        { name: 'LEVEL', min: -30, max: 6, val: 0, unit: 'dB' }
      ]
    },
    {
      id: 'delay', name: 'Stereo Delay', shortName: 'DLY', family: 'knobs',
      params: [
        { name: 'TIME', min: 20, max: 1000, val: 375, unit: 'ms' },
        { name: 'FEEDBACK', min: 0, max: 85, val: 35, unit: '%' },
        { name: 'HI CUT', min: 1, max: 16, val: 4.5, unit: 'kHz' },
        { name: 'LO CUT', min: 20, max: 500, val: 120, unit: 'Hz' },
        { name: 'SPREAD', min: 0, max: 100, val: 80, unit: '%' },
        { name: 'MIX', min: 0, max: 100, val: 45, unit: '%' }
      ]
    },
    {
      id: 'chorus', name: 'Stereo Chorus', shortName: 'CHR', family: 'knobs',
      params: [
        { name: 'SPEED', min: 0.1, max: 5.0, val: 1.4, unit: 'Hz' },
        { name: 'DEPTH', min: 5, max: 100, val: 55, unit: '%' },
        { name: 'DELAY', min: 2, max: 30, val: 12, unit: 'ms' },
        { name: 'PHASE', min: 0, max: 180, val: 90, unit: '°' },
        { name: 'TONE', min: -12, max: 12, val: 2, unit: 'dB' },
        { name: 'MIX', min: 0, max: 100, val: 50, unit: '%' }
      ]
    }
  ];

  // --- Global Application State ---
  const state = {
    isRotated90: false,
    stageW: 1024,
    stageH: 500,
    showTopNavBar: true,
    currentScreen: 0, // 0=Home, 1=Detail, 2=Tracks, 3=Effects, 4=Scenes, 5..8=Placeholder, 9=Setup
    currentBank: 0,   // 0=Ch1-8, 1=Ch9-16, 4=Aux1-8, 5=FX1L-4R, 6=Bus1-8, 7=Bus9-16, 8=Mtx-Main, 9=DCA1-8
    selectedChannel: 0, // 0..15
    detailSubTab: 0,  // 0=CONFIG, 1=GATE, 2=DYN, 3=EQ, 4=SENDS, 5=NAMING, 6=PRESET
    selectedEqBand: 1, // 0=HPF, 1=LOW, 2=LOMID, 3=HIMID, 4=HIGH
    rtaEnabled: true,
    sendsOnFaderActive: false,
    sofTargetBus: 12, // 0..15 (12 = Bus 13 FX1)
    muteGroups: [false, false, false, false, false, false],
    autoMixX: false,
    autoMixY: false,
    lockMutes: false,
    selectedFxSlot: 0, // 0..3
    larcSubPreset: 0,
    selectedSceneIdx: 0,

    // Audio & Transport
    audioCtx: null,
    isPlaying: false,
    isLooping: true,
    playStartCtxTime: 0,
    playheadSec: 0,
    totalDurationSec: 0,
    loopStartNorm: 0.0,
    loopEndNorm: 1.0,
    timelineZoom: 1.0,
    timelineOffset: 0.0,
    importWindowConfig: (() => {
      try {
        const saved = localStorage.getItem('mps_lite_import_window');
        if (saved) return JSON.parse(saved);
      } catch (e) {}
      return { startMin: 0, startSec: 0, durMin: 5, durSec: 0, fullTrack: false };
    })(),
    rawFiles: [],
    activeSection: {
      startPct: 0,
      endPct: 25,
      label: '0% - 25% (Intro)'
    },
    // iOS Streaming Engine & Waveform Peaks
    waveformPeaks: null,
    trackPeaks: [],
    bufferLookaheadSec: 25.0,
    isIosStreamerActive: true,

    // Channels (0..15 Input Channels)
    channels: [],
    // Aux (0..7), FX Returns (0..7), Buses (0..15), Matrices (0..5), DCAs (0..7), MainLR
    auxes: [],
    fxReturns: [],
    buses: [],
    matrices: [],
    dcas: [],
    main: {
      faderDb: 0.0,
      mute: false,
      solo: false,
      peakL: -90,
      peakR: -90,
      holdL: -90,
      holdR: -90,
      holdTimerL: 0,
      holdTimerR: 0,
      clip: false
    },
    fxSlots: [],
    scenes: [],
    clipboardCh: null,
    channelPairLinked: [false, false, false, false, false, false, false, false]
  };

  // --- PWA / Pin to Home Screen Management ---
  let deferredInstallPrompt = null;
  window.addEventListener('beforeinstallprompt', (e) => {
    e.preventDefault();
    deferredInstallPrompt = e;
    const autoBox = document.getElementById('pinAndroidAutoBox');
    if (autoBox) autoBox.style.display = 'block';
    const manualBox = document.getElementById('pinAndroidManualGuide');
    if (manualBox) manualBox.style.display = 'none';
  });

  window.addEventListener('appinstalled', () => {
    deferredInstallPrompt = null;
    localStorage.setItem('mps_lite_pin_prompt_done', 'true');
    showToast('✓ ปักหมุดลงหน้าจอโฮมสำเร็จแล้ว!');
    const ov = document.getElementById('mpsModalOverlay');
    if (ov) ov.style.display = 'none';
  });

  function isRunningStandalone() {
    const isStandaloneMedia = window.matchMedia && window.matchMedia('(display-mode: standalone)').matches;
    const isIOSStandalone = window.navigator.standalone === true;
    const isAndroidApp = document.referrer && document.referrer.includes('android-app://');
    return !!(isStandaloneMedia || isIOSStandalone || isAndroidApp);
  }

  function detectUserPlatform() {
    const ua = navigator.userAgent || '';
    const isIOS = /iPad|iPhone|iPod/.test(ua) || (navigator.platform === 'MacIntel' && navigator.maxTouchPoints > 1);
    const isAndroid = /Android/i.test(ua);
    return { isIOS, isAndroid };
  }

  // Register Service Worker for PWA
  if ('serviceWorker' in navigator) {
    window.addEventListener('load', () => {
      navigator.serviceWorker.register('/sw.js').catch((err) => {
        console.warn('SW registration warning:', err);
      });
    });
  }

  // Initialize 16 Input Channels
  for (let i = 0; i < kMaxTracks; i++) {
    state.channels.push({
      index: i,
      name: 'CH ' + String(i + 1).padStart(2, '0'),
      color: kChannelPalette[i % kChannelPalette.length],
      sourceTrack: i,
      fileName: '',
      buffer: null,
      sourceNode: null,
      peaks: null,
      chunks: new Map(),
      activeChunkNodes: [],

      // Preamp / Config
      trimDb: 0.0,
      phaseInvert: false,
      phantom48v: false,
      linked: false,
      delayOn: false,
      delayMs: 0.3,
      hpfOn: false,
      hpfFreq: 80.0,

      // Gate
      gateOn: false,
      gateThreshDb: -45.0,
      gateRangeDb: 40.0,
      gateAttackMs: 10.0,
      gateHoldMs: 50.0,
      gateReleaseMs: 150.0,
      gateFiltFreq: 250.0,
      gateGrDb: 0.0,

      // Dynamics Compressor
      compOn: false,
      compThreshDb: -20.0,
      compRatio: 4.0,
      compMakeupDb: 0.0,
      compKneeDb: 2.0,
      compAttackMs: 15.0,
      compHoldMs: 10.0,
      compReleaseMs: 150.0,
      compMixPct: 100.0,
      compGrDb: 0.0,

      // 4-Band Parametric EQ
      eqOn: true,
      eqLowType: 'lowshelf', eqLowFreq: 100.0, eqLowGainDb: 0.0, eqLowQ: 1.0,
      eqMid1Type: 'peq', eqMid1Freq: 500.0, eqMid1GainDb: 0.0, eqMid1Q: 1.4,
      eqMid2Type: 'peq', eqMid2Freq: 2500.0, eqMid2GainDb: 0.0, eqMid2Q: 1.4,
      eqHighType: 'highshelf', eqHighFreq: 8000.0, eqHighGainDb: 0.0, eqHighQ: 1.0,

      // Fader, Pan, Mute, Solo, Groups
      pan: 0.0,
      faderDb: 0.0,
      mute: false,
      solo: false,
      dcaMask: 0,
      muteGroupMask: 0,
      sendsDb: new Array(16).fill(-90.0),
      sendsMute: new Array(16).fill(false),

      // Realtime Meter state
      peakDb: -90.0,
      holdDb: -90.0,
      holdTimer: 0,
      clip: false,

      // Web Audio Nodes
      nodes: null
    });
  }

  // --- Stereo Channel Linking (Matches JUCE MixerParams::channelPairLinked) ---
  function isChannelLinked(chIdx) {
    if (chIdx < 0 || chIdx >= kMaxTracks) return false;
    const p = Math.floor(chIdx / 2);
    return !!state.channelPairLinked[p];
  }

  function getLinkedChannelIndex(chIdx) {
    return (chIdx % 2 === 0) ? (chIdx + 1) : (chIdx - 1);
  }

  function syncLinkedChannel(ch, fn) {
    if (!ch || !isChannelLinked(ch.index)) return;
    const otherIdx = getLinkedChannelIndex(ch.index);
    const otherCh = state.channels[otherIdx];
    if (otherCh) {
      fn(otherCh);
      syncChannelAudioParams(otherCh);
    }
  }

  function toggleChannelLink(chIdx) {
    if (chIdx < 0 || chIdx >= kMaxTracks) return;
    const pairIdx = Math.floor(chIdx / 2);
    const willLink = !state.channelPairLinked[pairIdx];
    state.channelPairLinked[pairIdx] = willLink;

    const chA_idx = pairIdx * 2;
    const chB_idx = pairIdx * 2 + 1;
    const chA = state.channels[chA_idx];
    const chB = state.channels[chB_idx];

    chA.linked = willLink;
    chB.linked = willLink;

    if (willLink) {
      // Determine source: If toggled while on chB, copy from chB; else from chA
      const src = (chIdx === chB_idx) ? chB : chA;
      const dst = (chIdx === chB_idx) ? chA : chB;

      // Stereo Pan: Ch A = Hard Left (-1.0), Ch B = Hard Right (+1.0)
      chA.pan = -1.0;
      chB.pan = 1.0;

      // Copy All Parameters from Source to Destination (Matches DetailScreen.cpp linkButton)
      dst.faderDb = src.faderDb;
      dst.mute = src.mute;
      dst.solo = src.solo;
      dst.trimDb = src.trimDb;
      dst.delayOn = src.delayOn;
      dst.delayMs = src.delayMs;
      dst.hpfOn = src.hpfOn;
      dst.hpfFreq = src.hpfFreq;

      dst.eqOn = src.eqOn;
      dst.eqLowType = src.eqLowType || 'lowshelf'; dst.eqLowFreq = src.eqLowFreq; dst.eqLowGainDb = src.eqLowGainDb; dst.eqLowQ = src.eqLowQ !== undefined ? src.eqLowQ : 1.0;
      dst.eqMid1Type = src.eqMid1Type || 'peq'; dst.eqMid1Freq = src.eqMid1Freq; dst.eqMid1GainDb = src.eqMid1GainDb; dst.eqMid1Q = src.eqMid1Q !== undefined ? src.eqMid1Q : 1.4;
      dst.eqMid2Type = src.eqMid2Type || 'peq'; dst.eqMid2Freq = src.eqMid2Freq; dst.eqMid2GainDb = src.eqMid2GainDb; dst.eqMid2Q = src.eqMid2Q !== undefined ? src.eqMid2Q : 1.4;
      dst.eqHighType = src.eqHighType || 'highshelf'; dst.eqHighFreq = src.eqHighFreq; dst.eqHighGainDb = src.eqHighGainDb; dst.eqHighQ = src.eqHighQ !== undefined ? src.eqHighQ : 1.0;

      dst.gateOn = src.gateOn;
      dst.gateThreshDb = src.gateThreshDb;
      dst.gateRangeDb = src.gateRangeDb;
      dst.gateAttackMs = src.gateAttackMs;
      dst.gateHoldMs = src.gateHoldMs;
      dst.gateReleaseMs = src.gateReleaseMs;
      dst.gateFiltFreq = src.gateFiltFreq;

      dst.compOn = src.compOn;
      dst.compThreshDb = src.compThreshDb;
      dst.compRatio = src.compRatio;
      dst.compMakeupDb = src.compMakeupDb;
      dst.compKneeDb = src.compKneeDb;
      dst.compAttackMs = src.compAttackMs;
      dst.compHoldMs = src.compHoldMs;
      dst.compReleaseMs = src.compReleaseMs;
      dst.compMixPct = src.compMixPct;

      for (let s = 0; s < 16; s++) {
        dst.sendsDb[s] = src.sendsDb[s];
        dst.sendsMute[s] = src.sendsMute[s];
      }
      showToast(`เชื่อมต่อ Stereo Link: CH ${chA_idx + 1} & CH ${chB_idx + 1} เรียบร้อย`);
    } else {
      chA.pan = 0.0;
      chB.pan = 0.0;
      showToast(`ยกเลิก Stereo Link: CH ${chA_idx + 1} & CH ${chB_idx + 1}`);
    }

    syncChannelAudioParams(chA);
    syncChannelAudioParams(chB);
    syncSoloAndMuteRouting();
  }

  for (let i = 0; i < 8; i++) {
    state.auxes.push({ name: 'Aux' + (i + 1), faderDb: 0.0, pan: 0.0, mute: false, solo: false, peakDb: -90, holdDb: -90, holdTimer: 0 });
    const fxIdx = Math.floor(i / 2) + 1;
    const side = i % 2 === 0 ? 'L' : 'R';
    state.fxReturns.push({ name: 'FX' + fxIdx + side, faderDb: 0.0, pan: i % 2 === 0 ? -1 : 1, mute: false, solo: false, peakDb: -90, holdDb: -90, holdTimer: 0 });
    state.dcas.push({ name: 'DCA ' + (i + 1), faderDb: 0.0, mute: false, solo: false, peakDb: -90, holdDb: -90, holdTimer: 0 });
  }
  for (let i = 0; i < 16; i++) {
    const isFx = i >= 12;
    state.buses.push({
      name: isFx ? ('FX ' + (i - 11) + ' Send') : ('Bus' + (i + 1)),
      faderDb: 0.0, pan: 0.0, mute: false, solo: false, peakDb: -90, holdDb: -90, holdTimer: 0
    });
  }
  for (let i = 0; i < 6; i++) {
    state.matrices.push({ name: 'Mtx' + (i + 1), faderDb: 0.0, pan: 0.0, mute: false, solo: false, peakDb: -90, holdDb: -90, holdTimer: 0 });
  }

  // Initialize 4 FX Slots
  for (let s = 0; s < kMaxFxSlots; s++) {
    const model = JSON.parse(JSON.stringify(kFxCatalog[s]));
    state.fxSlots.push({
      slotIndex: s,
      model: model,
      bypass: false,
      insertBus: 13 + s,
      inPeakL: 0, inPeakR: 0, outPeakL: 0, outPeakR: 0,
      nodes: null
    });
  }

  // Initialize Default Scenes
  state.scenes = [
    { name: '01: Default Initial State (Flat)', snapshot: null },
    { name: '02: X32 scene.scn (Thai Folk Band - Ponglang/Pin/Kan)', snapshot: 'preset_folk' },
    { name: '03: Punchy Rock Studio Mix', snapshot: 'preset_rock' },
    { name: '04: Warm Vocal & Lush Hall Reverb', snapshot: 'preset_vocal' }
  ];

  // ============================================================================
  // VIEWPORT AUTO-SCALING & TOUCH LOCK (No Page Scroll Ever, 100% Uniform Scaling)
  // ============================================================================
  state.forcePortrait = false;

  function fitStageToViewport() {
    const stage = document.getElementById('mps-stage');
    const rotOverlay = document.getElementById('mpsOrientationOverlay');
    if (!stage) return;

    const vv = window.visualViewport;
    const vw = vv ? vv.width : window.innerWidth;
    const vh = vv ? vv.height : window.innerHeight;

    const isPortrait = vw < vh;

    // Show orientation guide on mobile devices held in portrait mode
    if (isPortrait && !state.forcePortrait && vw < 768) {
      if (rotOverlay) rotOverlay.style.display = 'flex';
    } else {
      if (rotOverlay) rotOverlay.style.display = 'none';
    }

    state.isRotated90 = false;
    const baseH = 540;
    const aspect = vw / Math.max(1, vh);
    // Responsive width matching wide console aspect ratios (1024px to 1360px wide)
    const baseW = Math.max(1024, Math.min(1360, Math.round(baseH * Math.max(1.33, aspect))));

    stage.style.width = baseW + 'px';
    stage.style.height = baseH + 'px';

    // Strictly UNIFORM scaling — Never stretch or distort circles, text, or faders!
    const scale = Math.min(vw / baseW, vh / baseH);
    const ox = Math.round((vw - baseW * scale) / 2);
    const oy = Math.round((vh - baseH * scale) / 2);

    stage.style.transform = `translate(${ox}px, ${oy}px) scale(${scale})`;
  }

  // Convert pointer/touch event to normalized (0..1) coordinates inside an element
  function getNormCoords(e, el) {
    const pt = (e.touches && e.touches.length > 0) ? e.touches[0] :
               (e.changedTouches && e.changedTouches.length > 0) ? e.changedTouches[0] : e;
    const rect = el.getBoundingClientRect();
    return {
      x: Math.max(0, Math.min(1, (pt.clientX - rect.left) / Math.max(1, rect.width))),
      y: Math.max(0, Math.min(1, (pt.clientY - rect.top) / Math.max(1, rect.height))),
      rawX: pt.clientX,
      rawY: pt.clientY
    };
  }

  // Helper to attach lock-free drag interaction to any canvas/element without scrolling page
  function bindDrag(el, onDown, onMove, onUp, onDblClick) {
    if (!el) return;
    let dragging = false;
    let lastTap = 0;

    const handleStart = (e) => {
      if (e.cancelable) e.preventDefault();
      e.stopPropagation();
      const now = Date.now();
      if (onDblClick && (now - lastTap < 280)) {
        onDblClick(getNormCoords(e, el), e);
        lastTap = 0;
        return;
      }
      lastTap = now;
      dragging = true;
      if (onDown) onDown(getNormCoords(e, el), e);
    };

    const handleMove = (e) => {
      if (!dragging) return;
      if (e.cancelable) e.preventDefault();
      e.stopPropagation();
      if (onMove) onMove(getNormCoords(e, el), e);
    };

    const handleEnd = (e) => {
      if (!dragging) return;
      dragging = false;
      if (onUp) onUp(e);
    };

    el.addEventListener('mousedown', handleStart);
    window.addEventListener('mousemove', handleMove);
    window.addEventListener('mouseup', handleEnd);

    el.addEventListener('touchstart', handleStart, { passive: false });
    window.addEventListener('touchmove', handleMove, { passive: false });
    window.addEventListener('touchend', handleEnd);
    window.addEventListener('touchcancel', handleEnd);
  }

  // ============================================================================
  // WEB AUDIO DSP ENGINE (16 Channels + 4 FX Slots + Master Stereo Bus)
  // ============================================================================
  function ensureAudioEngine() {
    if (state.audioCtx) {
      if (state.audioCtx.state === 'suspended') {
        state.audioCtx.resume();
      }
      return state.audioCtx;
    }

    const AC = window.AudioContext || window.webkitAudioContext;
    let ctx;
    try {
      ctx = new AC({ sampleRate: 48000 });
    } catch (e) {
      ctx = new AC();
    }
    state.audioCtx = ctx;

    // Master Stereo Summing & Limiter
    state.masterSumNode = ctx.createGain();
    state.masterLimiterNode = ctx.createDynamicsCompressor();
    state.masterLimiterNode.threshold.value = -0.5;
    state.masterLimiterNode.knee.value = 0;
    state.masterLimiterNode.ratio.value = 20;
    state.masterLimiterNode.attack.value = 0.001;
    state.masterLimiterNode.release.value = 0.05;

    state.masterFaderNode = ctx.createGain();
    state.masterSplitter = ctx.createChannelSplitter(2);
    state.masterAnalyserL = ctx.createAnalyser();
    state.masterAnalyserR = ctx.createAnalyser();
    state.masterAnalyserL.fftSize = 256;
    state.masterAnalyserR.fftSize = 256;

    // RTA Spectrum Analyser for selected channel
    state.rtaAnalyser = ctx.createAnalyser();
    state.rtaAnalyser.fftSize = 1024;
    state.rtaAnalyser.smoothingTimeConstant = 0.72;

    state.masterSumNode.connect(state.masterLimiterNode);
    state.masterLimiterNode.connect(state.masterFaderNode);
    state.masterFaderNode.connect(ctx.destination);
    state.masterFaderNode.connect(state.masterSplitter);
    state.masterSplitter.connect(state.masterAnalyserL, 0);
    state.masterSplitter.connect(state.masterAnalyserR, 1);

    // Setup 4 FX Slots
    setupFxNodes(ctx);

    // Setup 16 Channel Strips
    state.channels.forEach(ch => setupChannelAudioNodes(ctx, ch));
    syncAllAudioParams();

    const statusEl = document.getElementById('lblTransStatus');
    if (statusEl) {
      statusEl.textContent = `Out: ${(ctx.sampleRate / 1000).toFixed(1)} kHz  Buf: 100%`;
    }
    return ctx;
  }

  function createReverbImpulse(ctx, decaySec, isPlate) {
    const sr = ctx.sampleRate;
    const len = Math.max(sr * 0.2, Math.min(sr * 6.0, Math.floor(sr * decaySec)));
    const buf = ctx.createBuffer(2, len, sr);
    const L = buf.getChannelData(0);
    const R = buf.getChannelData(1);
    for (let i = 0; i < len; i++) {
      const t = i / len;
      const env = Math.pow(1 - t, isPlate ? 2.4 : 3.2);
      L[i] = (Math.random() * 2 - 1) * env * 0.5;
      R[i] = (Math.random() * 2 - 1) * env * 0.5;
    }
    return buf;
  }

  function setupFxNodes(ctx) {
    state.fxSlots.forEach((slot, idx) => {
      const inGain = ctx.createGain();
      const outGain = ctx.createGain();
      const inAnalyser = ctx.createAnalyser();
      const outAnalyser = ctx.createAnalyser();
      inAnalyser.fftSize = 128;
      outAnalyser.fftSize = 128;

      inGain.connect(inAnalyser);

      if (idx === 0 || idx === 1) {
        // Hall / Plate Reverb
        const preDelay = ctx.createDelay(0.5);
        preDelay.delayTime.value = (slot.model.params[0].val || 20) / 1000;
        const conv = ctx.createConvolver();
        conv.buffer = createReverbImpulse(ctx, slot.model.params[1].val || 2.5, idx === 1);
        const damp = ctx.createBiquadFilter();
        damp.type = 'lowpass';
        damp.frequency.value = (slot.model.params[3].val || 7) * 1000;

        inGain.connect(preDelay);
        preDelay.connect(conv);
        conv.connect(damp);
        damp.connect(outGain);
        slot.nodes = { inGain, outGain, inAnalyser, outAnalyser, preDelay, conv, damp };
      } else if (idx === 2) {
        // Stereo Delay
        const dly = ctx.createDelay(2.0);
        dly.delayTime.value = (slot.model.params[0].val || 375) / 1000;
        const fb = ctx.createGain();
        fb.gain.value = (slot.model.params[1].val || 35) / 100;
        const lp = ctx.createBiquadFilter();
        lp.type = 'lowpass';
        lp.frequency.value = (slot.model.params[2].val || 4.5) * 1000;

        inGain.connect(dly);
        dly.connect(lp);
        lp.connect(fb);
        fb.connect(dly);
        lp.connect(outGain);
        slot.nodes = { inGain, outGain, inAnalyser, outAnalyser, dly, fb, lp };
      } else {
        // Stereo Chorus
        const chrDelay = ctx.createDelay(0.1);
        chrDelay.delayTime.value = 0.012;
        const lfo = ctx.createOscillator();
        lfo.frequency.value = slot.model.params[0].val || 1.4;
        const lfoDepth = ctx.createGain();
        lfoDepth.gain.value = 0.003;
        lfo.connect(lfoDepth);
        lfoDepth.connect(chrDelay.delayTime);
        lfo.start();

        inGain.connect(chrDelay);
        chrDelay.connect(outGain);
        slot.nodes = { inGain, outGain, inAnalyser, outAnalyser, chrDelay, lfo, lfoDepth };
      }

      outGain.connect(outAnalyser);
      outGain.connect(state.masterSumNode);
    });
  }

  function setupChannelAudioNodes(ctx, ch) {
    const trim = ctx.createGain();
    const delay = ctx.createDelay(1.0);
    const hpf = ctx.createBiquadFilter();
    hpf.type = 'highpass';
    hpf.Q.value = 0.707;

    // Noise Gate processor with True Bypass (Zero CPU when Gate is OFF)
    let gateGainVal = 1.0;
    let gateHoldCount = 0;
    ch.gateGrDb = 0.0;
    const gateBypass = ctx.createGain();
    const gateWet = ctx.createGain();
    const gateNode = ctx.createScriptProcessor ? ctx.createScriptProcessor(2048, 2, 2) : ctx.createGain();
    if (gateNode.onaudioprocess !== undefined) {
      gateNode.onaudioprocess = (e) => {
        if (!ch.gateOn) {
          ch.gateGrDb = 0.0;
          return;
        }

        const inL = e.inputBuffer.getChannelData(0);
        const inR = e.inputBuffer.numberOfChannels > 1 ? e.inputBuffer.getChannelData(1) : inL;
        const outL = e.outputBuffer.getChannelData(0);
        const outR = e.outputBuffer.numberOfChannels > 1 ? e.outputBuffer.getChannelData(1) : null;

        const thresh = ch.gateThreshDb;
        const rangeGain = Math.pow(10, -Math.abs(ch.gateRangeDb) / 20.0);
        const sr = ctx.sampleRate || 48000;
        const attCoeff = Math.exp(-1.0 / (Math.max(0.1, ch.gateAttackMs) * 0.001 * sr));
        const relCoeff = Math.exp(-1.0 / (Math.max(1.0, ch.gateReleaseMs) * 0.001 * sr));
        const holdSamples = Math.round(ch.gateHoldMs * 0.001 * sr);

        for (let i = 0; i < inL.length; i++) {
          const sL = inL[i];
          const sR = inR[i];
          const absS = Math.max(Math.abs(sL), Math.abs(sR));
          const db = absS > 1e-5 ? 20 * Math.log10(absS) : -100;
          if (db >= thresh) {
            gateHoldCount = holdSamples;
            gateGainVal = 1.0 + attCoeff * (gateGainVal - 1.0);
          } else if (gateHoldCount > 0) {
            gateHoldCount--;
          } else {
            gateGainVal = rangeGain + relCoeff * (gateGainVal - rangeGain);
          }
          outL[i] = sL * gateGainVal;
          if (outR) outR[i] = sR * gateGainVal;
        }
        ch.gateGrDb = gateGainVal < 0.99 ? 20 * Math.log10(Math.max(1e-4, gateGainVal)) : 0.0;
      };
    }

    const eqLow = ctx.createBiquadFilter();
    eqLow.type = 'lowshelf';
    const eqMid1 = ctx.createBiquadFilter();
    eqMid1.type = 'peaking';
    const eqMid2 = ctx.createBiquadFilter();
    eqMid2.type = 'peaking';
    const eqHigh = ctx.createBiquadFilter();
    eqHigh.type = 'highshelf';

    // Dynamics Compressor + Wet/Dry Split
    const comp = ctx.createDynamicsCompressor();
    const compWetGain = ctx.createGain();
    const compBypassGain = ctx.createGain();
    const makeup = ctx.createGain();

    const panner = ctx.createStereoPanner ? ctx.createStereoPanner() : ctx.createGain();
    const fader = ctx.createGain();
    const analyser = ctx.createAnalyser();
    analyser.fftSize = 256;

    const fxSendGains = [];
    for (let s = 0; s < kMaxFxSlots; s++) {
      const sg = ctx.createGain();
      sg.gain.value = 0;
      fader.connect(sg);
      if (state.fxSlots[s] && state.fxSlots[s].nodes) {
        sg.connect(state.fxSlots[s].nodes.inGain);
      }
      fxSendGains.push(sg);
    }

    // Signal Flow: Trim -> Delay -> HPF -> (Gate Bypass || Gate Wet) -> EQ (4-Band) -> Compressor -> Makeup -> Panner -> Fader
    trim.connect(delay);
    delay.connect(hpf);

    hpf.connect(gateBypass);
    gateBypass.connect(eqLow);

    hpf.connect(gateNode);
    gateNode.connect(gateWet);
    gateWet.connect(eqLow);

    eqLow.connect(eqMid1);
    eqMid1.connect(eqMid2);
    eqMid2.connect(eqHigh);

    // Parallel Compressor Split
    eqHigh.connect(comp);
    eqHigh.connect(compBypassGain);
    comp.connect(compWetGain);
    compWetGain.connect(makeup);
    compBypassGain.connect(makeup);

    makeup.connect(analyser);
    makeup.connect(panner);
    panner.connect(fader);
    fader.connect(state.masterSumNode);

    ch.nodes = {
      trim, delay, hpf, gateBypass, gateWet, gateNode,
      eqLow, eqMid1, eqMid2, eqHigh,
      comp, compWetGain, compBypassGain, makeup,
      panner, fader, analyser, fxSendGains
    };
  }

  let autoSaveSessionTimer = null;
  function triggerAutoSaveSession() {
    clearTimeout(autoSaveSessionTimer);
    autoSaveSessionTimer = setTimeout(() => {
      try {
        const snap = captureSnapshotData();
        localStorage.setItem('mps_lite_auto_session', JSON.stringify(snap));
      } catch (e) {}
    }, 400);
  }

  function syncChannelAudioParams(ch) {
    if (!state.audioCtx || !ch.nodes) return;
    const t = state.audioCtx.currentTime;
    const n = ch.nodes;

    const pol = ch.phaseInvert ? -1.0 : 1.0;
    n.trim.gain.setTargetAtTime(dbToGain(ch.trimDb) * pol, t, 0.01);
    n.delay.delayTime.setTargetAtTime(ch.delayOn ? (ch.delayMs / 1000.0) : 0.0, t, 0.01);

    n.hpf.frequency.setTargetAtTime(ch.hpfOn ? ch.hpfFreq : 10.0, t, 0.01);

    // Noise Gate Routing: True Bypass when Gate OFF
    if (ch.gateOn) {
      n.gateBypass.gain.setTargetAtTime(0.0, t, 0.01);
      n.gateWet.gain.setTargetAtTime(1.0, t, 0.01);
    } else {
      n.gateBypass.gain.setTargetAtTime(1.0, t, 0.01);
      n.gateWet.gain.setTargetAtTime(0.0, t, 0.01);
      ch.gateGrDb = 0.0;
    }

    const mapEqFilterType = (t) => {
      if (t === 'lowcut' || t === 'highpass') return 'highpass';
      if (t === 'hicut' || t === 'lowpass') return 'lowpass';
      if (t === 'lowshelf') return 'lowshelf';
      if (t === 'highshelf') return 'highshelf';
      return 'peaking'; // 'peq'
    };

    const lowType = mapEqFilterType(ch.eqLowType || 'lowshelf');
    if (n.eqLow.type !== lowType) n.eqLow.type = lowType;
    n.eqLow.frequency.setTargetAtTime(ch.eqLowFreq, t, 0.01);
    n.eqLow.Q.setTargetAtTime(ch.eqLowQ !== undefined ? ch.eqLowQ : (lowType === 'highpass' ? 0.707 : 1.0), t, 0.01);
    n.eqLow.gain.setTargetAtTime(ch.eqOn ? ch.eqLowGainDb : 0.0, t, 0.01);

    const mid1Type = mapEqFilterType(ch.eqMid1Type || 'peq');
    if (n.eqMid1.type !== mid1Type) n.eqMid1.type = mid1Type;
    n.eqMid1.frequency.setTargetAtTime(ch.eqMid1Freq, t, 0.01);
    n.eqMid1.Q.setTargetAtTime(ch.eqMid1Q !== undefined ? ch.eqMid1Q : 1.4, t, 0.01);
    n.eqMid1.gain.setTargetAtTime(ch.eqOn ? ch.eqMid1GainDb : 0.0, t, 0.01);

    const mid2Type = mapEqFilterType(ch.eqMid2Type || 'peq');
    if (n.eqMid2.type !== mid2Type) n.eqMid2.type = mid2Type;
    n.eqMid2.frequency.setTargetAtTime(ch.eqMid2Freq, t, 0.01);
    n.eqMid2.Q.setTargetAtTime(ch.eqMid2Q !== undefined ? ch.eqMid2Q : 1.4, t, 0.01);
    n.eqMid2.gain.setTargetAtTime(ch.eqOn ? ch.eqMid2GainDb : 0.0, t, 0.01);

    const highType = mapEqFilterType(ch.eqHighType || 'highshelf');
    if (n.eqHigh.type !== highType) n.eqHigh.type = highType;
    n.eqHigh.frequency.setTargetAtTime(ch.eqHighFreq, t, 0.01);
    n.eqHigh.Q.setTargetAtTime(ch.eqHighQ !== undefined ? ch.eqHighQ : (highType === 'lowpass' ? 0.707 : 1.0), t, 0.01);
    n.eqHigh.gain.setTargetAtTime(ch.eqOn ? ch.eqHighGainDb : 0.0, t, 0.01);

    // Dynamics Compressor routing with True Bypass & Parallel Mix
    if (ch.compOn) {
      const wet = Math.max(0, Math.min(1, (ch.compMixPct !== undefined ? ch.compMixPct : 100) / 100.0));
      const dry = 1.0 - wet;
      n.compWetGain.gain.setTargetAtTime(wet, t, 0.01);
      n.compBypassGain.gain.setTargetAtTime(dry, t, 0.01);

      n.comp.threshold.setTargetAtTime(ch.compThreshDb, t, 0.01);
      n.comp.ratio.setTargetAtTime(ch.compRatio, t, 0.01);
      n.comp.knee.setTargetAtTime(ch.compKneeDb, t, 0.01);
      n.comp.attack.setTargetAtTime(Math.max(0.001, ch.compAttackMs / 1000.0), t, 0.01);
      n.comp.release.setTargetAtTime(Math.max(0.01, ch.compReleaseMs / 1000.0), t, 0.01);
      n.makeup.gain.setTargetAtTime(dbToGain(ch.compMakeupDb), t, 0.01);
    } else {
      n.compWetGain.gain.setTargetAtTime(0.0, t, 0.01);
      n.compBypassGain.gain.setTargetAtTime(1.0, t, 0.01);
      n.makeup.gain.setTargetAtTime(1.0, t, 0.01);
    }

    if (n.panner.pan) {
      n.panner.pan.setTargetAtTime(ch.pan, t, 0.01);
    }

    // FX Sends (Buses 13..16 correspond to sendsDb[12..15])
    for (let s = 0; s < kMaxFxSlots; s++) {
      const sDb = ch.sendsMute[12 + s] ? -90.0 : ch.sendsDb[12 + s];
      n.fxSendGains[s].gain.setTargetAtTime(dbToGain(sDb), t, 0.01);
    }
    triggerAutoSaveSession();
  }

  function syncSoloAndMuteRouting() {
    if (!state.audioCtx) return;
    const t = state.audioCtx.currentTime;
    const anySolo = state.channels.some(c => c.solo);

    state.channels.forEach(ch => {
      if (!ch.nodes) return;
      let muted = ch.mute;
      // Check Mute Groups 1..6
      for (let mg = 0; mg < 6; mg++) {
        if ((ch.muteGroupMask & (1 << mg)) && state.muteGroups[mg]) {
          muted = true;
          break;
        }
      }
      // Check DCA mutes and offsets
      let dcaDbOffset = 0.0;
      for (let d = 0; d < 8; d++) {
        if (ch.dcaMask & (1 << d)) {
          if (state.dcas[d].mute) muted = true;
          dcaDbOffset += state.dcas[d].faderDb;
        }
      }
      if (anySolo && !ch.solo) muted = true;

      const effDb = ch.faderDb + dcaDbOffset;
      const gain = muted ? 0.0 : dbToGain(effDb);
      ch.nodes.fader.gain.setTargetAtTime(gain, t, 0.01);
    });

    if (state.masterFaderNode) {
      const mGain = state.main.mute ? 0.0 : dbToGain(state.main.faderDb);
      state.masterFaderNode.gain.setTargetAtTime(mGain, t, 0.01);
    }

    // Connect RTA to selected channel
    const selCh = state.channels[state.selectedChannel];
    if (selCh && selCh.nodes && state.rtaAnalyser) {
      try {
        state.channels.forEach(c => {
          if (c.nodes) try { c.nodes.makeup.disconnect(state.rtaAnalyser); } catch (e) {}
        });
        selCh.nodes.makeup.connect(state.rtaAnalyser);
      } catch (e) {}
    }
    triggerAutoSaveSession();
  }

  function syncAllAudioParams() {
    state.channels.forEach(ch => syncChannelAudioParams(ch));
    syncSoloAndMuteRouting();
  }

  // ============================================================================
  // iOS & WEB HIGH-PERFORMANCE MULTITRACK STREAMING & PEAK ENGINE
  // Solves iOS WebKit memory limits by maintaining a 25s rolling buffer
  // and extracting lightweight ~1.6KB waveform peaks for instant Canvas rendering
  // ============================================================================
  function computeBufferPeaks(audioBuffer, numPoints = 800) {
    if (!audioBuffer) return new Float32Array(numPoints * 2);
    const d0 = audioBuffer.getChannelData(0);
    const d1 = audioBuffer.numberOfChannels > 1 ? audioBuffer.getChannelData(1) : d0;
    const len = d0.length;
    const peaks = new Float32Array(numPoints * 2);
    const step = Math.max(1, Math.floor(len / numPoints));
    for (let p = 0; p < numPoints; p++) {
      const start = p * step;
      const end = Math.min(len, start + step);
      let min = 1.0, max = -1.0;
      const stride = Math.max(1, Math.floor((end - start) / 32));
      for (let i = start; i < end; i += stride) {
        const v = (d0[i] + d1[i]) * 0.5;
        if (v < min) min = v;
        if (v > max) max = v;
      }
      if (min > max) { min = 0; max = 0; }
      peaks[p * 2] = min;
      peaks[p * 2 + 1] = max;
    }
    return peaks;
  }

  function combinePeaks(peakArrays, numPoints = 800) {
    const res = new Float32Array(numPoints * 2);
    if (!peakArrays || !peakArrays.length) return res;
    for (let p = 0; p < numPoints; p++) {
      let min = 0;
      let max = 0;
      for (let i = 0; i < peakArrays.length; i++) {
        const arr = peakArrays[i];
        if (!arr || arr.length < numPoints * 2) continue;
        min = Math.min(min, arr[p * 2]);
        max = Math.max(max, arr[p * 2 + 1]);
      }
      res[p * 2] = min;
      res[p * 2 + 1] = max;
    }
    return res;
  }

  const MpsChunkStreamer = (() => {
    const CHUNK_LEN = 5.0; // 5-second chunks
    const LOOKAHEAD_CHUNKS = 5; // 25 seconds lookahead in RAM
    let worker = null;
    let reqId = 0;
    const callbacks = new Map();

    function getWorker() {
      if (!worker && window.Worker) {
        try {
          worker = new Worker('/audio-worker.js');
          worker.onmessage = (e) => {
            const { id, type, error } = e.data;
            const cb = callbacks.get(id);
            if (cb) {
              callbacks.delete(id);
              if (type === 'WORKER_ERROR') cb(new Error(error));
              else cb(null, e.data);
            }
          };
          worker.onerror = (err) => {
            console.warn('Audio worker background error, fallback active:', err);
          };
        } catch (e) {
          console.warn('Worker could not be started, inline fallback active:', e);
          worker = null;
        }
      }
      return worker;
    }

    async function extractPeaksAsync(file, trackSlot, numPoints = 800) {
      const w = getWorker();
      if (w) {
        return new Promise(async (resolve) => {
          try {
            const ab = await file.arrayBuffer();
            const id = ++reqId;
            callbacks.set(id, (err, res) => {
              if (err || !res) resolve(null);
              else resolve(res);
            });
            w.postMessage({
              type: 'GENERATE_PEAKS',
              id,
              data: { arrayBuffer: ab, numPoints, trackSlot }
            }, [ab]);
          } catch (e) {
            resolve(null);
          }
        });
      }
      // Inline Fallback
      try {
        const ab = await file.arrayBuffer();
        const ctx = ensureAudioEngine();
        const audioBuf = await ctx.decodeAudioData(ab.slice(0));
        const peaks = computeBufferPeaks(audioBuf, numPoints);
        return { peaks, durationSec: audioBuf.duration, sampleRate: audioBuf.sampleRate, channels: audioBuf.numberOfChannels };
      } catch (e) {
        return null;
      }
    }

    return {
      CHUNK_LEN,
      LOOKAHEAD_CHUNKS,
      extractPeaksAsync
    };
  })();

  // ============================================================================
  // MULTITRACK TRANSPORT & DEMO STEM GENERATOR
  // ============================================================================
  function startPlaybackFrom(offsetSec) {
    const ctx = ensureAudioEngine();
    const hasAudio = state.channels.some(c => c.buffer !== null);
    if (!hasAudio) {
      if (state.rawFiles && state.rawFiles.some(f => f)) {
        showToast('กำลังตัดเสียงท่อนเพื่อเริ่มเล่น...');
        applyAudioSection(state.activeSection.startPct || 0, state.activeSection.endPct || 25, state.activeSection.label || '0% - 25% (Intro)')
          .then(() => startPlaybackFrom(0));
        return;
      }
      loadDemoStems().then(() => startPlaybackFrom(0));
      return;
    }

    stopSourcesOnly();

    state.totalDurationSec = Math.max(1, ...state.channels.map(c => c.buffer ? c.buffer.duration : 0));
    const loopStartSec = state.loopStartNorm * state.totalDurationSec;
    const loopEndSec = Math.max(loopStartSec + 0.2, state.loopEndNorm * state.totalDurationSec);

    let startPos = Math.max(0, Math.min(offsetSec, state.totalDurationSec - 0.05));
    if (state.isLooping && (startPos < loopStartSec || startPos >= loopEndSec)) {
      startPos = loopStartSec;
    }

    state.playheadSec = startPos;
    state.playStartCtxTime = ctx.currentTime - startPos;
    state.isPlaying = true;

    state.channels.forEach(ch => {
      if (!ch.buffer || !ch.nodes) return;
      const src = ctx.createBufferSource();
      src.buffer = ch.buffer;
      if (state.isLooping) {
        src.loop = true;
        src.loopStart = Math.min(loopStartSec, ch.buffer.duration - 0.05);
        src.loopEnd = Math.min(loopEndSec, ch.buffer.duration);
      }
      src.connect(ch.nodes.trim);
      if (startPos < ch.buffer.duration) {
        src.start(0, startPos);
      }
      ch.sourceNode = src;
    });

    updateTransportUi();
  }

  function stopSourcesOnly() {
    state.channels.forEach(ch => {
      if (ch.sourceNode) {
        try { ch.sourceNode.stop(); } catch (e) {}
        try { ch.sourceNode.disconnect(); } catch (e) {}
        ch.sourceNode = null;
      }
    });
  }

  function getCurrentPlayheadSec() {
    if (!state.isPlaying || !state.audioCtx || state.totalDurationSec <= 0) {
      return state.playheadSec;
    }
    const elapsed = state.audioCtx.currentTime - state.playStartCtxTime;
    const loopStartSec = state.loopStartNorm * state.totalDurationSec;
    const loopEndSec = Math.max(loopStartSec + 0.2, state.loopEndNorm * state.totalDurationSec);
    if (state.isLooping) {
      const span = loopEndSec - loopStartSec;
      if (elapsed >= loopStartSec) {
        return loopStartSec + ((elapsed - loopStartSec) % span);
      }
    }
    if (elapsed >= state.totalDurationSec) {
      state.isPlaying = false;
      state.playheadSec = 0;
      stopSourcesOnly();
      updateTransportUi();
      return 0;
    }
    return elapsed;
  }

  function updateTransportUi() {
    const btnPlay = document.getElementById('btnTransPlay');
    const btnLoop = document.getElementById('btnTransLoop');
    const btnSec = document.getElementById('btnSectionMenu');
    if (btnPlay) btnPlay.classList.toggle('active', state.isPlaying);
    if (btnLoop) btnLoop.classList.toggle('active', state.isLooping);
    if (btnSec && state.activeSection) {
      btnSec.innerHTML = `✂️ ท่อน: <b>${state.activeSection.label.split(' ')[0]}</b> ▼`;
    }
  }

  // Synthesize 8 Realistic Studio Stems in < 200ms using OfflineAudioContext
  async function loadDemoStems() {
    const ctx = ensureAudioEngine();
    const sr = ctx.sampleRate;
    const bpm = 112;
    const beat = 60 / bpm;
    const bar = beat * 4;
    const bars = 4;
    const dur = bar * bars;
    const frames = Math.floor(sr * dur);

    const demoDefs = [
      { ch: 0, name: 'Kick', color: '#e53935', type: 'kick' },
      { ch: 1, name: 'Snare', color: '#fb8c00', type: 'snare' },
      { ch: 2, name: 'HiHat', color: '#fdd835', type: 'hihat' },
      { ch: 3, name: 'Tom/Perc', color: '#ffb300', type: 'perc' },
      { ch: 4, name: 'Bass DI', color: '#1e88e5', type: 'bass' },
      { ch: 5, name: 'E.Piano', color: '#43a047', type: 'keys' },
      { ch: 6, name: 'Rhythm Gtr', color: '#00acc1', type: 'gtr' },
      { ch: 7, name: 'Lead Synth', color: '#8e24aa', type: 'lead' }
    ];

    for (const def of demoDefs) {
      const off = new OfflineAudioContext(2, frames, sr);
      for (let b = 0; b < bars; b++) {
        const b0 = b * bar;
        if (def.type === 'kick') {
          [0, 1.5, 2.0, 3.25].forEach(bt => {
            const t = b0 + bt * beat;
            const osc = off.createOscillator();
            const g = off.createGain();
            osc.frequency.setValueAtTime(145, t);
            osc.frequency.exponentialRampToValueAtTime(46, t + 0.09);
            g.gain.setValueAtTime(0.95, t);
            g.gain.exponentialRampToValueAtTime(0.001, t + 0.26);
            osc.connect(g); g.connect(off.destination);
            osc.start(t); osc.stop(t + 0.28);
          });
        } else if (def.type === 'snare') {
          [1, 3].forEach(bt => {
            const t = b0 + bt * beat;
            const osc = off.createOscillator();
            const g = off.createGain();
            osc.frequency.setValueAtTime(185, t);
            g.gain.setValueAtTime(0.55, t);
            g.gain.exponentialRampToValueAtTime(0.001, t + 0.14);
            osc.connect(g); g.connect(off.destination);
            osc.start(t); osc.stop(t + 0.15);

            const nBuf = off.createBuffer(1, Math.floor(sr * 0.18), sr);
            const d = nBuf.getChannelData(0);
            for (let i = 0; i < d.length; i++) d[i] = Math.random() * 2 - 1;
            const ns = off.createBufferSource(); ns.buffer = nBuf;
            const hp = off.createBiquadFilter(); hp.type = 'highpass'; hp.frequency.value = 1100;
            const ng = off.createGain();
            ng.gain.setValueAtTime(0.7, t);
            ng.gain.exponentialRampToValueAtTime(0.001, t + 0.17);
            ns.connect(hp); hp.connect(ng); ng.connect(off.destination);
            ns.start(t); ns.stop(t + 0.18);
          });
        } else if (def.type === 'hihat') {
          for (let s = 0; s < 16; s++) {
            const t = b0 + s * (beat * 0.25);
            const nBuf = off.createBuffer(1, Math.floor(sr * 0.06), sr);
            const d = nBuf.getChannelData(0);
            for (let i = 0; i < d.length; i++) d[i] = Math.random() * 2 - 1;
            const ns = off.createBufferSource(); ns.buffer = nBuf;
            const hp = off.createBiquadFilter(); hp.type = 'highpass'; hp.frequency.value = 7800;
            const g = off.createGain();
            g.gain.setValueAtTime(s % 4 === 2 ? 0.45 : 0.22, t);
            g.gain.exponentialRampToValueAtTime(0.001, t + 0.05);
            ns.connect(hp); hp.connect(g); g.connect(off.destination);
            ns.start(t); ns.stop(t + 0.06);
          }
        } else if (def.type === 'perc') {
          [0.75, 2.75, 3.75].forEach((bt, idx) => {
            const t = b0 + bt * beat;
            const osc = off.createOscillator();
            const g = off.createGain();
            osc.frequency.setValueAtTime(idx === 2 ? 290 : 220, t);
            osc.frequency.exponentialRampToValueAtTime(110, t + 0.12);
            g.gain.setValueAtTime(0.5, t);
            g.gain.exponentialRampToValueAtTime(0.001, t + 0.14);
            osc.connect(g); g.connect(off.destination);
            osc.start(t); osc.stop(t + 0.15);
          });
        } else if (def.type === 'bass') {
          const notes = [
            { bt: 0, f: 87.31, d: 0.7 }, { bt: 1.5, f: 87.31, d: 0.4 },
            { bt: 2, f: 98.0, d: 0.65 }, { bt: 3, f: 110.0, d: 0.45 }, { bt: 3.5, f: 130.81, d: 0.4 }
          ];
          notes.forEach(n => {
            const t = b0 + n.bt * beat;
            const osc = off.createOscillator(); osc.type = 'sawtooth';
            osc.frequency.setValueAtTime(n.f, t);
            const lp = off.createBiquadFilter(); lp.type = 'lowpass';
            lp.frequency.setValueAtTime(750, t);
            lp.frequency.exponentialRampToValueAtTime(220, t + n.d * beat);
            const g = off.createGain();
            g.gain.setValueAtTime(0.65, t);
            g.gain.exponentialRampToValueAtTime(0.001, t + n.d * beat);
            osc.connect(lp); lp.connect(g); g.connect(off.destination);
            osc.start(t); osc.stop(t + n.d * beat + 0.02);
          });
        } else if (def.type === 'keys') {
          const chords = [
            [174.61, 220, 261.63, 329.63],
            [164.81, 196, 246.94, 293.66],
            [146.83, 174.61, 220, 261.63],
            [130.81, 164.81, 196, 246.94]
          ];
          chords[b % 4].forEach(f => {
            const osc = off.createOscillator(); osc.type = 'triangle';
            osc.frequency.setValueAtTime(f, b0);
            const g = off.createGain();
            g.gain.setValueAtTime(0.22, b0);
            g.gain.exponentialRampToValueAtTime(0.01, b0 + bar * 0.95);
            osc.connect(g); g.connect(off.destination);
            osc.start(b0); osc.stop(b0 + bar);
          });
        } else if (def.type === 'gtr') {
          [0.5, 1.5, 2.5, 3.5].forEach(bt => {
            const t = b0 + bt * beat;
            [349.23, 440, 523.25].forEach(f => {
              const osc = off.createOscillator(); osc.type = 'sawtooth';
              osc.frequency.setValueAtTime(f, t);
              const bp = off.createBiquadFilter(); bp.type = 'bandpass'; bp.frequency.value = 1200;
              const g = off.createGain();
              g.gain.setValueAtTime(0.18, t);
              g.gain.exponentialRampToValueAtTime(0.001, t + 0.16);
              osc.connect(bp); bp.connect(g); g.connect(off.destination);
              osc.start(t); osc.stop(t + 0.17);
            });
          });
        } else if (def.type === 'lead') {
          const melody = [
            { bt: 0.5, f: 523.25 }, { bt: 1.5, f: 587.33 },
            { bt: 2.25, f: 659.25 }, { bt: 3.0, f: 523.25 }
          ];
          melody.forEach(m => {
            const t = b0 + m.bt * beat;
            const osc = off.createOscillator(); osc.type = 'sine';
            osc.frequency.setValueAtTime(m.f, t);
            const g = off.createGain();
            g.gain.setValueAtTime(0.35, t);
            g.gain.exponentialRampToValueAtTime(0.001, t + beat * 0.7);
            osc.connect(g); g.connect(off.destination);
            osc.start(t); osc.stop(t + beat * 0.75);
          });
        }
      }
      const rendered = await off.startRendering();
      const ch = state.channels[def.ch];
      ch.buffer = rendered;
      ch.name = def.name;
      ch.color = def.color;
      ch.fileName = `Demo_${def.name.replace(/\s+/g, '')}.wav`;
      ch.peaks = computeBufferPeaks(rendered, 800);
    }

    const activePeaks = state.channels.map(c => c.peaks).filter(p => p);
    state.waveformPeaks = combinePeaks(activePeaks, 800);
    state.totalDurationSec = dur;
    state.loopStartNorm = 0.0;
    state.loopEndNorm = 1.0;
    drawMainTimeline();
    renderCurrentScreen();
  }

  // ============================================================================
  // CUSTOM CANVAS WIDGETS (Exact JUCE LookAndFeel Replicas)
  // ============================================================================

  // 1. 16-Segment LedMeter (Matches LedMeter.cpp)
  const kMeterSegs = [
    { db: -60, on: '#00d455', off: '#05260f' },
    { db: -50, on: '#00d455', off: '#05260f' },
    { db: -42, on: '#00d455', off: '#05260f' },
    { db: -36, on: '#00d455', off: '#05260f' },
    { db: -30, on: '#00d455', off: '#05260f' },
    { db: -24, on: '#00e650', off: '#062d12' },
    { db: -20, on: '#10f040', off: '#083315' },
    { db: -18, on: '#38f538', off: '#0a3b18' },
    { db: -15, on: '#a8f020', off: '#243306' },
    { db: -12, on: '#ffea00', off: '#3b3600' },
    { db:  -9, on: '#ffd000', off: '#3b3000' },
    { db:  -6, on: '#ffa000', off: '#3b2400' },
    { db:  -4, on: '#ff7700', off: '#3b1b00' },
    { db:  -2, on: '#ff4422', off: '#3b1008' },
    { db:  -1, on: '#ff2222', off: '#3b0808' },
    { db:   0, on: '#ff1111', off: '#400505' }
  ];

  function drawLedMeterCanvas(canvas, dbL, holdL, dbR, holdR, isStereo, isClip) {
    if (!canvas) return;
    const dpr = window.devicePixelRatio || 1;
    const w = canvas.clientWidth || (isStereo ? 18 : 12);
    const h = canvas.clientHeight || 180;
    if (canvas.width !== Math.round(w * dpr) || canvas.height !== Math.round(h * dpr)) {
      canvas.width = Math.round(w * dpr);
      canvas.height = Math.round(h * dpr);
    }
    const g = canvas.getContext('2d');
    g.save();
    g.scale(dpr, dpr);
    g.clearRect(0, 0, w, h);

    // Top Clip LED circle
    const clipR = 3.0;
    const cx = w * 0.5;
    const cy = 4.0;
    g.beginPath();
    g.arc(cx, cy, clipR, 0, Math.PI * 2);
    g.fillStyle = isClip ? '#ff1744' : '#320a0a';
    g.fill();
    if (isClip) {
      g.strokeStyle = 'rgba(255,255,255,0.85)';
      g.lineWidth = 0.8;
      g.stroke();
    }

    const meterTop = 10.0;
    const meterH = h - meterTop - 2.0;
    if (meterH <= 10) { g.restore(); return; }

    const drawBar = (bx, bw, levelDb, peakHoldDb) => {
      g.fillStyle = '#080a0c';
      g.fillRect(bx, meterTop, bw, meterH);
      g.strokeStyle = '#20242a';
      g.lineWidth = 0.8;
      g.strokeRect(bx, meterTop, bw, meterH);

      const gap = 1.0;
      const totalH = meterH - 2.0;
      const segH = (totalH - 15 * gap) / 16.0;
      const segW = bw - 2.0;
      const leftX = bx + 1.0;

      for (let s = 0; s < 16; s++) {
        const sy = meterTop + meterH - 1.0 - (s + 1) * segH - s * gap;
        const isOn = levelDb >= kMeterSegs[s].db;
        const isPeak = peakHoldDb >= kMeterSegs[s].db && (s === 15 || peakHoldDb < kMeterSegs[s + 1].db);
        if (isOn) {
          g.fillStyle = kMeterSegs[s].on;
        } else if (isPeak) {
          g.fillStyle = s >= 14 ? '#ff2222' : '#ffffff';
        } else {
          g.fillStyle = kMeterSegs[s].off;
        }
        g.fillRect(leftX, sy, segW, Math.max(1.2, segH));
      }
    };

    if (!isStereo) {
      drawBar(1.0, w - 2.0, dbL, holdL);
    } else {
      const bw = (w - 3.0) * 0.5;
      drawBar(1.0, bw, dbL, holdL);
      drawBar(2.0 + bw, bw, dbR, holdR);
    }
    g.restore();
  }

  // 2. Vertical Console Fader Canvas (Matches ConsoleLookAndFeel::drawLinearSlider + ChannelStripComponent markings)
  function drawFaderCanvas(canvas, faderDb, isSendMode) {
    if (!canvas) return;
    const dpr = window.devicePixelRatio || 1;
    const w = canvas.clientWidth || 54;
    const h = canvas.clientHeight || 180;
    if (canvas.width !== Math.round(w * dpr) || canvas.height !== Math.round(h * dpr)) {
      canvas.width = Math.round(w * dpr);
      canvas.height = Math.round(h * dpr);
    }
    const g = canvas.getContext('2d');
    g.save();
    g.scale(dpr, dpr);
    g.clearRect(0, 0, w, h);

    const topPad = 12.0;
    const botPad = 12.0;
    const rangeH = Math.max(20.0, h - topPad - botPad);

    // Scale markings (+10, 0, -5, -10, -20, -30, -40, -60, -inf)
    const marks = [10, 0, -5, -10, -20, -30, -40, -60, -90];
    g.strokeStyle = '#606874';
    g.lineWidth = 1.0;
    for (const mDb of marks) {
      const pos = faderDbToPos(mDb);
      const y = (h - botPad) - pos * rangeH;
      g.beginPath();
      g.moveTo(2, y);
      g.lineTo(7, y);
      g.stroke();
      if (mDb === 0) {
        g.strokeStyle = 'rgba(0, 210, 255, 0.45)';
        g.beginPath();
        g.moveTo(8, y);
        g.lineTo(w - 4, y);
        g.stroke();
        g.strokeStyle = '#606874';
      }
    }

    // Fader slot track
    const trackW = 4.0;
    const trackX = w * 0.5 - trackW * 0.5;
    g.fillStyle = '#0a0c0e';
    g.fillRect(trackX, topPad - 2, trackW, rangeH + 4);
    g.strokeStyle = '#252930';
    g.lineWidth = 1.0;
    g.strokeRect(trackX, topPad - 2, trackW, rangeH + 4);

    // Tactile Fader Cap
    const pos = faderDbToPos(faderDb);
    const capW = Math.min(w - 6, 36);
    const capH = 24.0;
    const capX = w * 0.5 - capW * 0.5;
    const capY = (h - botPad) - pos * rangeH - capH * 0.5;

    const grad = g.createLinearGradient(capX, capY, capX, capY + capH);
    grad.addColorStop(0.0, '#484e58');
    grad.addColorStop(0.5, '#2e333b');
    grad.addColorStop(1.0, '#1e2126');
    g.fillStyle = grad;
    g.beginPath();
    g.roundRect(capX, capY, capW, capH, 3);
    g.fill();

    g.strokeStyle = '#5a626f';
    g.lineWidth = 1.0;
    g.stroke();

    // Center Cyan (or Amber for Sends on Faders) groove line
    g.fillStyle = isSendMode ? '#ffa000' : '#00e5ff';
    g.fillRect(capX + 4, capY + capH * 0.5 - 1.0, capW - 8, 2.0);

    g.restore();
  }

  // 3. Rotary Pan Slider on Channel Strip (Matches ConsoleLookAndFeel::drawRotarySlider)
  function drawMiniPanDial(canvas, panVal) {
    if (!canvas) return;
    const dpr = window.devicePixelRatio || 1;
    const w = canvas.clientWidth || 34;
    const h = canvas.clientHeight || 34;
    if (canvas.width !== Math.round(w * dpr) || canvas.height !== Math.round(h * dpr)) {
      canvas.width = Math.round(w * dpr);
      canvas.height = Math.round(h * dpr);
    }
    const g = canvas.getContext('2d');
    g.save();
    g.scale(dpr, dpr);
    g.clearRect(0, 0, w, h);

    const cx = w * 0.5;
    const cy = h * 0.5;
    const r = Math.min(w, h) * 0.44;
    const startA = Math.PI * 0.75;
    const endA = Math.PI * 2.25;
    const prop = (panVal + 1.0) * 0.5;
    const curA = startA + prop * (endA - startA);

    // Background arc
    g.beginPath();
    g.arc(cx, cy, r - 2, startA, endA);
    g.strokeStyle = '#282d35';
    g.lineWidth = 2.8;
    g.lineCap = 'round';
    g.stroke();

    // Active cyan arc
    g.beginPath();
    g.arc(cx, cy, r - 2, startA, curA);
    g.strokeStyle = '#00d2ff';
    g.lineWidth = 3.0;
    g.stroke();

    // Metallic body
    const dr = r - 5.5;
    const grad = g.createLinearGradient(cx, cy - dr, cx, cy + dr);
    grad.addColorStop(0, '#3a404a');
    grad.addColorStop(1, '#1f2329');
    g.beginPath();
    g.arc(cx, cy, dr, 0, Math.PI * 2);
    g.fillStyle = grad;
    g.fill();
    g.strokeStyle = '#4f5865';
    g.lineWidth = 1.0;
    g.stroke();

    // Pointer notch
    const rotA = (prop - 0.5) * Math.PI * 1.5;
    g.translate(cx, cy);
    g.rotate(rotA);
    g.fillStyle = '#00d2ff';
    g.fillRect(-1.2, -dr + 1.5, 2.4, dr * 0.75);

    g.restore();
  }

  // 4. ConsoleDial Component on Detail Screen (Matches ConsoleDial.cpp)
  function createConsoleDialElement(opts) {
    const size = opts.size || 52;
    const wrap = document.createElement('div');
    wrap.style.width = size + 'px';
    wrap.style.height = size + 'px';
    wrap.style.position = 'relative';
    wrap.style.cursor = 'ns-resize';
    wrap.style.flexShrink = '0';

    const canvas = document.createElement('canvas');
    canvas.style.width = '100%';
    canvas.style.height = '100%';
    canvas.style.display = 'block';
    wrap.appendChild(canvas);

    let val = opts.value;
    const min = opts.min;
    const max = opts.max;
    const isLog = !!opts.isLog;
    const arcColor = opts.color || '#00d2ff';

    const valToProp = (v) => {
      const c = Math.max(min, Math.min(max, v));
      if (isLog && min > 0) {
        return (Math.log10(c) - Math.log10(min)) / (Math.log10(max) - Math.log10(min));
      }
      return (c - min) / Math.max(1e-6, max - min);
    };

    const propToVal = (p) => {
      const cp = Math.max(0, Math.min(1, p));
      if (isLog && min > 0) {
        const lMin = Math.log10(min);
        const lMax = Math.log10(max);
        return Math.pow(10, lMin + cp * (lMax - lMin));
      }
      return min + cp * (max - min);
    };

    const paint = () => {
      const dpr = window.devicePixelRatio || 1;
      const w = size;
      const h = size;
      canvas.width = Math.round(w * dpr);
      canvas.height = Math.round(h * dpr);
      const g = canvas.getContext('2d');
      g.save();
      g.scale(dpr, dpr);
      g.clearRect(0, 0, w, h);

      // Title
      g.fillStyle = '#e0e4ea';
      const titleFont = size <= 52 ? 'bold 8px Inter, sans-serif' : 'bold 9px Inter, sans-serif';
      g.font = titleFont;
      g.textAlign = 'center';
      g.fillText(opts.title || '', w * 0.5, size <= 52 ? 8 : 9);

      const topOff = size <= 52 ? 9 : 11;
      const availH = h - topOff;
      const dialR = Math.max(10, Math.min(w - 14, availH - 6) * 0.5);
      const cx = w * 0.5;
      const cy = topOff + availH * 0.5;

      const startA = Math.PI * 0.75;
      const endA = Math.PI * 2.25;
      const prop = valToProp(val);
      const curA = startA + prop * (endA - startA);

      // Background arc
      g.beginPath();
      g.arc(cx, cy, dialR + 2, startA, endA);
      g.strokeStyle = '#20242a';
      g.lineWidth = size <= 52 ? 2.2 : 2.8;
      g.lineCap = 'round';
      g.stroke();

      // Active arc
      g.beginPath();
      g.arc(cx, cy, dialR + 2, startA, curA);
      g.strokeStyle = arcColor;
      g.lineWidth = size <= 52 ? 2.4 : 3.0;
      g.stroke();

      // Gunmetal cap
      const grad = g.createLinearGradient(cx, cy - dialR, cx, cy + dialR);
      grad.addColorStop(0, '#404652');
      grad.addColorStop(1, '#1c2026');
      g.beginPath();
      g.arc(cx, cy, dialR, 0, Math.PI * 2);
      g.fillStyle = grad;
      g.fill();
      g.strokeStyle = '#586272';
      g.lineWidth = 1.0;
      g.stroke();

      // Inner dark face
      const inR = dialR * 0.70;
      g.beginPath();
      g.arc(cx, cy, inR, 0, Math.PI * 2);
      g.fillStyle = '#1a1d24';
      g.fill();
      g.strokeStyle = '#2b323e';
      g.stroke();

      // Pointer notch on rim
      g.save();
      g.translate(cx, cy);
      g.rotate((prop - 0.5) * Math.PI * 1.5);
      g.fillStyle = arcColor;
      g.fillRect(-1.0, -dialR + 0.5, 2.0, (dialR - inR) + 1);
      g.restore();

      // Digital readout inside face
      g.fillStyle = '#e6f0fa';
      const valFont = size <= 52 ? 'bold 7.5px Inter, sans-serif' : 'bold 8.5px Inter, sans-serif';
      g.font = valFont;
      g.textAlign = 'center';
      g.textBaseline = 'middle';
      const txt = opts.format ? opts.format(val) : val.toFixed(1);
      g.fillText(txt, cx, cy);

      g.restore();
    };

    let startProp = 0;
    let startY = 0;
    bindDrag(wrap,
      (coords) => {
        startProp = valToProp(val);
        startY = coords.y;
      },
      (coords) => {
        const dy = startY - coords.y;
        const newProp = Math.max(0, Math.min(1, startProp + dy * 1.3));
        val = propToVal(newProp);
        paint();
        if (opts.onChange) opts.onChange(val);
      },
      null,
      () => {
        if (opts.def !== undefined) {
          val = opts.def;
          paint();
          if (opts.onChange) opts.onChange(val);
        }
      }
    );

    setTimeout(paint, 0);
    return wrap;
  }

  // 5. TrapezoidPanIndicator (Matches TrapezoidPanIndicator.cpp)
  function drawTrapezoidPan(canvas, panVal) {
    if (!canvas) return;
    const dpr = window.devicePixelRatio || 1;
    const w = canvas.clientWidth || 80;
    const h = canvas.clientHeight || 24;
    canvas.width = Math.round(w * dpr);
    canvas.height = Math.round(h * dpr);
    const g = canvas.getContext('2d');
    g.save();
    g.scale(dpr, dpr);

    g.fillStyle = '#121417';
    g.fillRect(0, 0, w, h);
    g.strokeStyle = '#262b33';
    g.strokeRect(0.5, 0.5, w - 1, h - 1);

    const midX = w * 0.5;
    const topY = 2;
    const botY = h - 2;

    // Left Blue Wedge
    const blueGrad = g.createLinearGradient(2, topY, midX, botY);
    blueGrad.addColorStop(0, '#1976d2');
    blueGrad.addColorStop(1, '#0d2b45');
    g.fillStyle = blueGrad;
    g.beginPath();
    g.moveTo(2, topY);
    g.lineTo(midX - 1, botY);
    g.lineTo(2, botY);
    g.closePath();
    g.fill();

    // Right Orange Wedge
    const orangeGrad = g.createLinearGradient(w - 2, topY, midX, botY);
    orangeGrad.addColorStop(0, '#e65100');
    orangeGrad.addColorStop(1, '#3e1b00');
    g.fillStyle = orangeGrad;
    g.beginPath();
    g.moveTo(w - 2, topY);
    g.lineTo(w - 2, botY);
    g.lineTo(midX + 1, botY);
    g.closePath();
    g.fill();

    // White pointer line
    const norm = (panVal + 1.0) * 0.5;
    const cx = 2 + norm * (w - 4);
    g.strokeStyle = '#ffffff';
    g.lineWidth = 1.5;
    g.beginPath();
    g.moveTo(cx, 1);
    g.lineTo(cx, h - 1);
    g.stroke();

    g.restore();
  }

  // ============================================================================
  // TOP NAVIGATION BAR & BANK SELECTOR BAR RENDERING
  // ============================================================================
  const kNavTabs = [
    { name: 'Home', screen: 0, active: true, icon: '<path d="M8,2 L15,8 L13,8 L13,13 L3,13 L3,8 L1,8 Z" stroke-width="1.3"/>' },
    { name: 'Detail', screen: 1, active: true, icon: '<rect x="2" y="2" width="12" height="10" rx="1.5" stroke-width="1.2"/><line x1="4" y1="7" x2="12" y2="7" stroke-width="1.2"/><circle cx="5.5" cy="4.5" r="1.3" fill="currentColor"/><circle cx="10.5" cy="9.5" r="1.3" fill="currentColor"/>' },
    { name: 'Tracks', screen: 2, active: true, icon: '<path d="M1,7 Q4.5,1 8,7 T15,7" stroke-width="1.4"/>' },
    { name: 'Effects', screen: 3, active: true, icon: '<path d="M1,7 Q4.5,1 8,7 T15,7" stroke-width="1.4"/>' },
    { name: 'Scenes', screen: 4, active: true, icon: '<line x1="3" y1="2" x2="3" y2="12" stroke-width="1.2"/><rect x="1.5" y="4" width="3" height="2" fill="currentColor"/><line x1="8" y1="2" x2="8" y2="12" stroke-width="1.2"/><rect x="6.5" y="7" width="3" height="2" fill="currentColor"/><line x1="13" y1="2" x2="13" y2="12" stroke-width="1.2"/><rect x="11.5" y="5" width="3" height="2" fill="currentColor"/>' },
    { name: 'Meters', screen: 5, active: false, icon: '<rect x="2" y="6" width="2.2" height="6" fill="currentColor"/><rect x="5.5" y="3" width="2.2" height="9" fill="currentColor"/><rect x="9" y="1" width="2.2" height="11" fill="currentColor"/><rect x="12.5" y="5" width="2.2" height="7" fill="currentColor"/>' },
    { name: 'Recorder', screen: 6, active: false, icon: '<circle cx="5" cy="6" r="2.5" stroke-width="1.2"/><circle cx="11" cy="6" r="2.5" stroke-width="1.2"/><line x1="4" y1="11" x2="12" y2="11" stroke-width="1.2"/>' },
    { name: 'Routing', screen: 7, active: false, icon: '<circle cx="4" cy="3" r="1" fill="currentColor"/><circle cx="8" cy="3" r="1" fill="currentColor"/><circle cx="12" cy="3" r="1" fill="currentColor"/><circle cx="4" cy="7" r="1" fill="currentColor"/><circle cx="8" cy="7" r="1" fill="currentColor"/><circle cx="12" cy="7" r="1" fill="currentColor"/><circle cx="4" cy="11" r="1" fill="currentColor"/><circle cx="8" cy="11" r="1" fill="currentColor"/><circle cx="12" cy="11" r="1" fill="currentColor"/>' },
    { name: 'Monitor', screen: 8, active: false, icon: '<path d="M3,8 A5,5 0 0,1 13,8" stroke-width="1.3"/><rect x="2" y="7" width="2.2" height="4.5" rx="1" fill="currentColor"/><rect x="11.8" y="7" width="2.2" height="4.5" rx="1" fill="currentColor"/>' },
    { name: 'Setup', screen: 9, active: true, icon: '<circle cx="8" cy="7" r="4.5" stroke-width="1.8"/><circle cx="8" cy="7" r="1.8" fill="currentColor"/>' }
  ];

  function renderTopNav() {
    const topBar = document.getElementById('topNavBar');
    const container = document.getElementById('navTabsContainer');
    if (!topBar || !container) return;

    topBar.classList.toggle('hidden', !state.showTopNavBar);
    container.innerHTML = '';

    kNavTabs.forEach(tab => {
      const btn = document.createElement('button');
      btn.className = `mps-nav-btn ${state.currentScreen === tab.screen ? 'active' : ''} ${!tab.active ? 'roadmap' : ''}`;
      btn.innerHTML = `<svg viewBox="0 0 16 14">${tab.icon}</svg><span>${tab.name}</span>`;
      btn.onclick = () => showScreen(tab.screen);
      container.appendChild(btn);
    });

    const secBtn = document.getElementById('topSectionSlicerBtn');
    if (secBtn && state.activeSection) {
      secBtn.innerHTML = `<span>✂️</span> <span>ท่อน: <b>${state.activeSection.label.split(' ')[0]}</b></span> <span style="font-size:9px;opacity:0.8;">▼</span>`;
    }
  }

  function createBankSelectorBar(showBackToMixerBtn) {
    const bar = document.createElement('div');
    bar.className = 'mps-bank-bar';

    const logoBox = document.createElement('div');
    logoBox.className = 'bank-logo-box';
    if (showBackToMixerBtn) {
      const backBtn = document.createElement('button');
      backBtn.className = 'cbtn';
      backBtn.style.cssText = 'width:100%;height:100%;background:#202630;color:#00d2ff;font-size:10.5px;font-weight:800;';
      backBtn.textContent = '< Mixer';
      backBtn.onclick = () => showScreen(0);
      logoBox.appendChild(backBtn);
    } else {
      logoBox.innerHTML = `<div class="bank-logo-title">MPS</div><div class="bank-logo-sub">LITE 16CH</div>`;
    }
    bar.appendChild(logoBox);

    const btnsRow = document.createElement('div');
    btnsRow.className = 'bank-buttons-row';

    const bankDefs = [
      { idx: 0, label: 'Ch 1-8', locked: false },
      { idx: 1, label: 'Ch 9-16', locked: false },
      { idx: 2, label: 'Ch 17-24', locked: true },
      { idx: 3, label: 'Ch 25-32', locked: true },
      { idx: 4, label: 'Aux 1-8', locked: false },
      { idx: 5, label: 'FX 1L-4R', locked: false },
      { idx: 6, label: 'Bus 1-8', locked: false },
      { idx: 7, label: 'Bus 9-16', locked: false },
      { idx: 8, label: 'Mtx-Main', locked: false },
      { idx: 9, label: 'DCA 1-8', locked: false }
    ];

    bankDefs.forEach(b => {
      const btn = document.createElement('div');
      btn.className = `bank-btn-item ${state.currentBank === b.idx ? 'active' : ''} ${b.locked ? 'lite-locked' : ''}`;
      btn.innerHTML = `
        <canvas class="bank-mini-meters" id="bankMeterCanvas-${b.idx}"></canvas>
        <div class="bank-btn-label">${b.label}</div>
      `;
      btn.onclick = () => {
        if (b.locked) {
          showToast('รุ่น Practice Lite รองรับแทร็กอินพุตสูงสุด 16 ช่อง (Ch 1-8 และ Ch 9-16)');
          return;
        }
        state.currentBank = b.idx;
        if (b.idx === 0 && state.selectedChannel >= 8) state.selectedChannel = 0;
        if (b.idx === 1 && state.selectedChannel < 8) state.selectedChannel = 8;
        renderCurrentScreen();
      };
      btnsRow.appendChild(btn);
    });

    bar.appendChild(btnsRow);
    return bar;
  }

  // ============================================================================
  // SCREEN 0: HOME SCREEN (MIXER CONSOLE)
  // ============================================================================
  function renderHomeScreen() {
    const screen = document.getElementById('screen-home');
    if (!screen) return;
    screen.innerHTML = '';

    // 1. Top Bank Selector Bar
    screen.appendChild(createBankSelectorBar(false));

    // 2. Main Body (Left Utility Panel + 8 Strips + Master Strip)
    const body = document.createElement('div');
    body.className = 'home-body';

    // Left Panel
    const leftPanel = document.createElement('div');
    leftPanel.className = 'home-left-panel';

    const sofBtn = document.createElement('button');
    sofBtn.className = `cbtn sof-btn ${state.sendsOnFaderActive ? 'active' : ''}`;
    sofBtn.textContent = 'SENDS ON FADERS';
    sofBtn.onclick = () => {
      state.sendsOnFaderActive = !state.sendsOnFaderActive;
      renderHomeScreen();
    };
    leftPanel.appendChild(sofBtn);

    const sofSelect = document.createElement('select');
    sofSelect.className = 'sof-select';
    for (let b = 0; b < 16; b++) {
      const opt = document.createElement('option');
      opt.value = b;
      opt.selected = state.sofTargetBus === b;
      opt.textContent = b < 12 ? `BUS ${b + 1} (MON ${b + 1})` : `BUS ${b + 1} (FX ${b - 11}: ${state.fxSlots[b - 12].model.shortName})`;
      sofSelect.appendChild(opt);
    }
    sofSelect.onchange = (e) => {
      state.sofTargetBus = parseInt(e.target.value, 10);
      if (state.sendsOnFaderActive) renderHomeScreen();
    };
    leftPanel.appendChild(sofSelect);

    const amLbl = document.createElement('div');
    amLbl.className = 'left-sec-title';
    amLbl.textContent = 'AUTO MIXING';
    leftPanel.appendChild(amLbl);

    const amRow = document.createElement('div');
    amRow.className = 'automix-row';
    amRow.innerHTML = `
      <button class="cbtn ${state.autoMixX ? 'active-cyan' : ''}" id="btnAutoX">X</button>
      <button class="cbtn ${state.autoMixY ? 'active-cyan' : ''}" id="btnAutoY">Y</button>
    `;
    leftPanel.appendChild(amRow);

    const mgLbl = document.createElement('div');
    mgLbl.className = 'left-sec-title';
    mgLbl.textContent = 'MUTE GROUPS';
    leftPanel.appendChild(mgLbl);

    const mgGrid = document.createElement('div');
    mgGrid.className = 'mutegroup-grid';
    for (let m = 0; m < 6; m++) {
      const mBtn = document.createElement('button');
      mBtn.className = `cbtn ${state.muteGroups[m] ? 'active-red' : ''}`;
      mBtn.textContent = String(m + 1);
      mBtn.onclick = () => {
        state.muteGroups[m] = !state.muteGroups[m];
        syncSoloAndMuteRouting();
        renderHomeScreen();
      };
      mgGrid.appendChild(mBtn);
    }
    leftPanel.appendChild(mgGrid);

    const botBtns = document.createElement('div');
    botBtns.className = 'left-bottom-btns';
    const clearSoloBtn = document.createElement('button');
    const anySolo = state.channels.some(c => c.solo);
    clearSoloBtn.className = `cbtn ${anySolo ? 'active-yellow' : ''}`;
    clearSoloBtn.textContent = 'SHOW SOLOS';
    clearSoloBtn.onclick = () => {
      if (anySolo) {
        state.channels.forEach(c => { c.solo = false; });
        syncSoloAndMuteRouting();
        renderHomeScreen();
      }
    };
    const lockMuteBtn = document.createElement('button');
    lockMuteBtn.className = `cbtn ${state.lockMutes ? 'active-red' : ''}`;
    lockMuteBtn.textContent = 'LOCK MUTES';
    lockMuteBtn.onclick = () => {
      state.lockMutes = !state.lockMutes;
      renderHomeScreen();
    };
    botBtns.appendChild(clearSoloBtn);
    botBtns.appendChild(lockMuteBtn);
    leftPanel.appendChild(botBtns);

    const projLbl = document.createElement('div');
    projLbl.className = 'left-sec-title';
    projLbl.textContent = 'PROJECT';
    leftPanel.appendChild(projLbl);

    const projRow = document.createElement('div');
    projRow.style.cssText = 'display:grid;grid-template-columns:1fr 1fr;gap:4px;width:100%;margin-top:2px;';
    projRow.innerHTML = `
      <button class="cbtn active-cyan" style="font-size:9.5px;font-weight:700;height:24px;padding:0;" onclick="MPS.saveProject()" title="Save Project (.mpsproj) to Downloads">💾 SAVE</button>
      <button class="cbtn" style="font-size:9.5px;font-weight:700;height:24px;padding:0;background:#202630;color:#00e5ff;border-color:#384454;" onclick="MPS.loadProject()" title="Open Project File (.mpsproj / .json)">📂 OPEN</button>
    `;
    leftPanel.appendChild(projRow);

    body.appendChild(leftPanel);

    // Center 8 Strips + Master Strip
    const stripsArea = document.createElement('div');
    stripsArea.className = 'home-strips-area';

    const eightGrid = document.createElement('div');
    eightGrid.className = 'home-eight-strips';

    for (let i = 0; i < 8; i++) {
      eightGrid.appendChild(createStripElementForBank(state.currentBank, i));
    }
    stripsArea.appendChild(eightGrid);

    // Right Master Strip (MainLR)
    const masterWrap = document.createElement('div');
    masterWrap.className = 'home-master-wrap';
    masterWrap.appendChild(createMasterStripElement());
    stripsArea.appendChild(masterWrap);

    body.appendChild(stripsArea);
    screen.appendChild(body);
  }

  function createStripElementForBank(bank, slotIdx) {
    // Bank 0 = Ch 1-8, Bank 1 = Ch 9-16
    if (bank === 0 || bank === 1) {
      const chIdx = bank * 8 + slotIdx;
      const ch = state.channels[chIdx];
      const isSelected = state.selectedChannel === chIdx;
      const isSof = state.sendsOnFaderActive;

      const isLnk = isChannelLinked(chIdx);
      const isLeftOfPair = chIdx % 2 === 0;
      const linkSuffix = isLeftOfPair ? 'L' : 'R';

      const strip = document.createElement('div');
      strip.className = `ch-strip ${isSelected ? 'selected' : ''} ${isLnk ? ('linked-strip ' + (isLeftOfPair ? 'link-left' : 'link-right')) : ''}`;
      strip.onclick = () => {
        if (state.selectedChannel !== chIdx) {
          state.selectedChannel = chIdx;
          syncSoloAndMuteRouting();
          renderHomeScreen();
        }
      };

      const bannerColor = isSof ? '#ffa000' : ch.color;
      const scribble = document.createElement('div');
      scribble.className = 'ch-scribble';
      scribble.style.background = bannerColor + '38';
      scribble.style.borderColor = isLnk ? '#00e676' : bannerColor;
      const srcTxt = isSof
        ? `SEND B${state.sofTargetBus + 1}`
        : (isLnk ? `LINK ${linkSuffix}` : (ch.buffer ? `IN ${chIdx + 1}` : 'OFF'));
      scribble.innerHTML = `
        <div class="ch-scribble-stripe" style="background:${bannerColor};"></div>
        <div class="ch-scribble-name">${ch.name}${isLnk ? ' <span style="color:#00e676;font-size:9px;" title="Stereo Linked">⚯</span>' : ''}</div>
        <div class="ch-scribble-src" style="${isLnk ? 'color:#00e676;font-weight:700;' : ''}">${srcTxt}</div>
      `;
      scribble.onclick = (e) => {
        e.stopPropagation();
        state.selectedChannel = chIdx;
        showScreen(1); // Open Detail Screen!
      };
      strip.appendChild(scribble);

      // Digital Preamp Trim Readout
      const trimBox = document.createElement('div');
      trimBox.className = 'ch-trim-box';
      trimBox.id = `homeTrimBox-${chIdx}`;
      const updateTrimText = () => {
        trimBox.textContent = (ch.trimDb >= 0 ? '+' : '') + ch.trimDb.toFixed(1) + ' dB';
      };
      updateTrimText();
      let startTrim = 0, startY = 0;
      bindDrag(trimBox,
        (c) => { startTrim = ch.trimDb; startY = c.rawY; },
        (c) => {
          const dy = startY - c.rawY;
          ch.trimDb = Math.max(-18, Math.min(18, Math.round((startTrim + dy * 0.2) * 2) / 2));
          updateTrimText();
          if (isChannelLinked(chIdx)) {
            const oIdx = getLinkedChannelIndex(chIdx);
            state.channels[oIdx].trimDb = ch.trimDb;
            syncChannelAudioParams(state.channels[oIdx]);
            const oTrimBox = document.getElementById(`homeTrimBox-${oIdx}`);
            if (oTrimBox) oTrimBox.textContent = trimBox.textContent;
          }
          syncChannelAudioParams(ch);
        },
        null,
        () => {
          ch.trimDb = 0;
          updateTrimText();
          if (isChannelLinked(chIdx)) {
            const oIdx = getLinkedChannelIndex(chIdx);
            state.channels[oIdx].trimDb = 0;
            syncChannelAudioParams(state.channels[oIdx]);
            const oTrimBox = document.getElementById(`homeTrimBox-${oIdx}`);
            if (oTrimBox) oTrimBox.textContent = trimBox.textContent;
          }
          syncChannelAudioParams(ch);
        }
      );
      strip.appendChild(trimBox);

      // Rotary Pan Dial
      const panWrap = document.createElement('div');
      panWrap.className = 'ch-pan-wrap';
      const panCanvas = document.createElement('canvas');
      panCanvas.className = 'ch-pan-canvas';
      panCanvas.id = `homePanCanvas-${chIdx}`;
      panWrap.appendChild(panCanvas);
      let startPan = 0, panStartY = 0;
      bindDrag(panWrap,
        (c) => { startPan = ch.pan; panStartY = c.rawY; },
        (c) => {
          const dy = panStartY - c.rawY;
          ch.pan = Math.max(-1.0, Math.min(1.0, startPan + dy * 0.015));
          drawMiniPanDial(panCanvas, ch.pan);
          if (isChannelLinked(chIdx)) {
            const oIdx = getLinkedChannelIndex(chIdx);
            state.channels[oIdx].pan = -ch.pan;
            syncChannelAudioParams(state.channels[oIdx]);
            const oPanC = document.getElementById(`homePanCanvas-${oIdx}`);
            if (oPanC) drawMiniPanDial(oPanC, state.channels[oIdx].pan);
          }
          syncChannelAudioParams(ch);
        },
        null,
        () => {
          ch.pan = isChannelLinked(chIdx) ? (chIdx % 2 === 0 ? -1.0 : 1.0) : 0.0;
          drawMiniPanDial(panCanvas, ch.pan);
          if (isChannelLinked(chIdx)) {
            const oIdx = getLinkedChannelIndex(chIdx);
            state.channels[oIdx].pan = -ch.pan;
            syncChannelAudioParams(state.channels[oIdx]);
            const oPanC = document.getElementById(`homePanCanvas-${oIdx}`);
            if (oPanC) drawMiniPanDial(oPanC, state.channels[oIdx].pan);
          }
          syncChannelAudioParams(ch);
        }
      );
      setTimeout(() => drawMiniPanDial(panCanvas, ch.pan), 0);
      strip.appendChild(panWrap);

      // Solo & Mute Row
      const smRow = document.createElement('div');
      smRow.className = 'ch-sm-row';
      const soloBtn = document.createElement('button');
      soloBtn.className = `ch-sm-btn solo ${ch.solo ? 'on' : ''}`;
      soloBtn.textContent = 'SOLO';
      soloBtn.onclick = (e) => {
        e.stopPropagation();
        ch.solo = !ch.solo;
        if (isChannelLinked(chIdx)) {
          const oIdx = getLinkedChannelIndex(chIdx);
          state.channels[oIdx].solo = ch.solo;
        }
        syncSoloAndMuteRouting();
        renderHomeScreen();
      };

      let effMute = ch.mute;
      for (let mg = 0; mg < 6; mg++) {
        if ((ch.muteGroupMask & (1 << mg)) && state.muteGroups[mg]) effMute = true;
      }
      const muteBtn = document.createElement('button');
      muteBtn.className = `ch-sm-btn mute ${effMute ? 'on' : ''}`;
      muteBtn.textContent = 'MUTE';
      muteBtn.onclick = (e) => {
        e.stopPropagation();
        if (state.lockMutes) return showToast('ปุ่ม MUTE ถูกล็อกอยู่ (LOCK MUTES)');
        ch.mute = !ch.mute;
        if (isChannelLinked(chIdx)) {
          const oIdx = getLinkedChannelIndex(chIdx);
          state.channels[oIdx].mute = ch.mute;
        }
        syncSoloAndMuteRouting();
        renderHomeScreen();
      };
      smRow.appendChild(soloBtn);
      smRow.appendChild(muteBtn);
      strip.appendChild(smRow);

      // Fader + Meter Zone
      const fmZone = document.createElement('div');
      fmZone.className = 'ch-fader-meter-zone';
      const faderCanvas = document.createElement('canvas');
      faderCanvas.className = 'ch-fader-canvas';
      faderCanvas.id = `homeFaderCanvas-${chIdx}`;
      const meterCanvas = document.createElement('canvas');
      meterCanvas.className = 'ch-meter-canvas';
      meterCanvas.id = `homeMeterCanvas-${chIdx}`;
      fmZone.appendChild(faderCanvas);
      fmZone.appendChild(meterCanvas);
      strip.appendChild(fmZone);

      // Bottom dB Label
      const dbBox = document.createElement('div');
      dbBox.className = 'ch-db-box';
      dbBox.id = `homeDbBox-${chIdx}`;
      const curValDb = isSof ? ch.sendsDb[state.sofTargetBus] : ch.faderDb;
      dbBox.textContent = formatDbText(curValDb);
      strip.appendChild(dbBox);

      // Fader touch/mouse drag (Locked to Fader, zero page scroll)
      const updateFaderFromCoords = (coords) => {
        const pos = Math.max(0.0, Math.min(1.0, 1.0 - coords.y));
        const db = faderPosToDb(pos);
        if (isSof) {
          ch.sendsDb[state.sofTargetBus] = db;
          if (isChannelLinked(chIdx)) {
            const oIdx = getLinkedChannelIndex(chIdx);
            state.channels[oIdx].sendsDb[state.sofTargetBus] = db;
            syncChannelAudioParams(state.channels[oIdx]);
            const oFadC = document.getElementById(`homeFaderCanvas-${oIdx}`);
            const oDbBox = document.getElementById(`homeDbBox-${oIdx}`);
            if (oFadC) drawFaderCanvas(oFadC, db, isSof);
            if (oDbBox) oDbBox.textContent = formatDbText(db);
          }
          syncChannelAudioParams(ch);
        } else {
          ch.faderDb = db;
          if (isChannelLinked(chIdx)) {
            const oIdx = getLinkedChannelIndex(chIdx);
            state.channels[oIdx].faderDb = db;
            const oFadC = document.getElementById(`homeFaderCanvas-${oIdx}`);
            const oDbBox = document.getElementById(`homeDbBox-${oIdx}`);
            if (oFadC) drawFaderCanvas(oFadC, db, isSof);
            if (oDbBox) oDbBox.textContent = formatDbText(db);
          }
          syncSoloAndMuteRouting();
        }
        dbBox.textContent = formatDbText(db);
        drawFaderCanvas(faderCanvas, db, isSof);
      };

      bindDrag(faderCanvas, updateFaderFromCoords, updateFaderFromCoords, null, () => {
        const targetDb = 0.0;
        if (isSof) {
          ch.sendsDb[state.sofTargetBus] = targetDb;
          if (isChannelLinked(chIdx)) {
            const oIdx = getLinkedChannelIndex(chIdx);
            state.channels[oIdx].sendsDb[state.sofTargetBus] = targetDb;
            syncChannelAudioParams(state.channels[oIdx]);
            const oFadC = document.getElementById(`homeFaderCanvas-${oIdx}`);
            const oDbBox = document.getElementById(`homeDbBox-${oIdx}`);
            if (oFadC) drawFaderCanvas(oFadC, targetDb, isSof);
            if (oDbBox) oDbBox.textContent = formatDbText(targetDb);
          }
          syncChannelAudioParams(ch);
        } else {
          ch.faderDb = targetDb;
          if (isChannelLinked(chIdx)) {
            const oIdx = getLinkedChannelIndex(chIdx);
            state.channels[oIdx].faderDb = targetDb;
            const oFadC = document.getElementById(`homeFaderCanvas-${oIdx}`);
            const oDbBox = document.getElementById(`homeDbBox-${oIdx}`);
            if (oFadC) drawFaderCanvas(oFadC, targetDb, isSof);
            if (oDbBox) oDbBox.textContent = formatDbText(targetDb);
          }
          syncSoloAndMuteRouting();
        }
        dbBox.textContent = formatDbText(targetDb);
        drawFaderCanvas(faderCanvas, targetDb, isSof);
      });

      setTimeout(() => {
        drawFaderCanvas(faderCanvas, curValDb, isSof);
        drawLedMeterCanvas(meterCanvas, ch.peakDb, ch.holdDb, ch.peakDb, ch.holdDb, false, ch.clip);
      }, 0);

      return strip;
    }

    // Other Banks: Aux 1-8 (4), FX 1L-4R (5), Bus 1-8 (6), Bus 9-16 (7), Mtx-Main (8), DCA 1-8 (9)
    let entity = null;
    let bannerCol = '#00bcd4';
    let srcText = 'BUS OUT';
    if (bank === 4) { entity = state.auxes[slotIdx]; bannerCol = '#43a047'; srcText = 'AUX IN'; }
    else if (bank === 5) { entity = state.fxReturns[slotIdx]; bannerCol = '#d81b60'; srcText = 'FX RACK'; }
    else if (bank === 6) { entity = state.buses[slotIdx]; bannerCol = '#00bcd4'; srcText = 'BUS ' + (slotIdx + 1); }
    else if (bank === 7) { entity = state.buses[8 + slotIdx]; bannerCol = '#00bcd4'; srcText = 'BUS ' + (9 + slotIdx); }
    else if (bank === 8) {
      if (slotIdx < 6) { entity = state.matrices[slotIdx]; bannerCol = '#8e24aa'; srcText = 'MATRIX'; }
      else if (slotIdx === 6) { entity = { name: 'MainC', faderDb: 0, mute: false, peakDb: -90, holdDb: -90 }; bannerCol = '#78909c'; srcText = 'CENTER'; }
      else { return createMasterStripElement(); }
    } else {
      entity = state.dcas[slotIdx]; bannerCol = '#0288d1'; srcText = 'DCA GRP';
    }

    const strip = document.createElement('div');
    strip.className = 'ch-strip';
    strip.innerHTML = `
      <div class="ch-scribble" style="background:${bannerCol}38;border-color:${bannerCol};">
        <div class="ch-scribble-stripe" style="background:${bannerCol};"></div>
        <div class="ch-scribble-name">${entity.name}</div>
        <div class="ch-scribble-src">${srcText}</div>
      </div>
      <div class="ch-pan-wrap"><canvas class="ch-pan-canvas"></canvas></div>
      <div class="ch-sm-row">
        <button class="ch-sm-btn solo ${entity.solo ? 'on' : ''}">SOLO</button>
        <button class="ch-sm-btn mute ${entity.mute ? 'on' : ''}">MUTE</button>
      </div>
      <div class="ch-fader-meter-zone">
        <canvas class="ch-fader-canvas"></canvas>
        <canvas class="ch-meter-canvas"></canvas>
      </div>
      <div class="ch-db-box">${formatDbText(entity.faderDb)}</div>
    `;

    const panC = strip.querySelector('.ch-pan-canvas');
    const fadC = strip.querySelector('.ch-fader-canvas');
    const metC = strip.querySelector('.ch-meter-canvas');
    const dbLbl = strip.querySelector('.ch-db-box');
    const muteBtn = strip.querySelector('.ch-sm-btn.mute');

    muteBtn.onclick = () => {
      entity.mute = !entity.mute;
      syncSoloAndMuteRouting();
      renderHomeScreen();
    };

    const onFadMove = (coords) => {
      entity.faderDb = faderPosToDb(1.0 - coords.y);
      dbLbl.textContent = formatDbText(entity.faderDb);
      drawFaderCanvas(fadC, entity.faderDb, false);
      syncSoloAndMuteRouting();
    };
    bindDrag(fadC, onFadMove, onFadMove, null, () => {
      entity.faderDb = 0.0;
      dbLbl.textContent = formatDbText(0.0);
      drawFaderCanvas(fadC, 0.0, false);
      syncSoloAndMuteRouting();
    });

    setTimeout(() => {
      drawMiniPanDial(panC, entity.pan || 0);
      drawFaderCanvas(fadC, entity.faderDb, false);
      drawLedMeterCanvas(metC, entity.peakDb || -90, entity.holdDb || -90, -90, -90, false, false);
    }, 0);

    return strip;
  }

  function createMasterStripElement() {
    const strip = document.createElement('div');
    strip.className = 'ch-strip';
    const bannerCol = '#e53935';
    strip.innerHTML = `
      <div class="ch-scribble" style="background:${bannerCol}38;border-color:${bannerCol};">
        <div class="ch-scribble-stripe" style="background:${bannerCol};"></div>
        <div class="ch-scribble-name">MainLR</div>
        <div class="ch-scribble-src">STEREO</div>
      </div>
      <div class="ch-sm-row master-sm">
        <button class="ch-sm-btn mute ${state.main.mute ? 'on' : ''}">MUTE</button>
      </div>
      <div class="ch-fader-meter-zone">
        <canvas class="ch-fader-canvas" id="masterFaderCanvas"></canvas>
        <canvas class="ch-meter-canvas stereo" id="masterMeterCanvas"></canvas>
      </div>
      <div class="ch-db-box" id="masterDbBox">${formatDbText(state.main.faderDb)}</div>
    `;

    const muteBtn = strip.querySelector('.ch-sm-btn.mute');
    muteBtn.onclick = () => {
      state.main.mute = !state.main.mute;
      syncSoloAndMuteRouting();
      renderHomeScreen();
    };

    const fadC = strip.querySelector('#masterFaderCanvas');
    const metC = strip.querySelector('#masterMeterCanvas');
    const dbBox = strip.querySelector('#masterDbBox');

    const updateMasterFad = (coords) => {
      state.main.faderDb = faderPosToDb(1.0 - coords.y);
      dbBox.textContent = formatDbText(state.main.faderDb);
      drawFaderCanvas(fadC, state.main.faderDb, false);
      syncSoloAndMuteRouting();
    };
    bindDrag(fadC, updateMasterFad, updateMasterFad, null, () => {
      state.main.faderDb = 0.0;
      dbBox.textContent = formatDbText(0.0);
      drawFaderCanvas(fadC, 0.0, false);
      syncSoloAndMuteRouting();
    });

    setTimeout(() => {
      drawFaderCanvas(fadC, state.main.faderDb, false);
      drawLedMeterCanvas(metC, state.main.peakL, state.main.holdL, state.main.peakR, state.main.holdR, true, state.main.clip);
    }, 0);

    return strip;
  }

  // ============================================================================
  // SCREEN 1: DETAIL SCREEN (CONFIG/PREAMP, GATE, DYN, EQ, SENDS, NAMING, PRESET)
  // ============================================================================
  function renderDetailScreen() {
    const screen = document.getElementById('screen-detail');
    if (!screen) return;
    screen.innerHTML = '';

    // 1. Top Bank Selector Bar with "< Mixer" button
    screen.appendChild(createBankSelectorBar(true));

    const ch = state.channels[state.selectedChannel];

    // 2. Detail Body (Left Preview Strip + Right Sub-Tabs & Card)
    const body = document.createElement('div');
    body.className = 'detail-body';

    // Left Preview Strip
    const isLnk = isChannelLinked(ch.index);
    const linkSuffix = ch.index % 2 === 0 ? 'L' : 'R';
    const nameTxt = isLnk ? `${ch.name} (LINK ${linkSuffix})` : ch.name;

    const leftStrip = document.createElement('div');
    leftStrip.className = 'detail-left-strip';
    leftStrip.innerHTML = `
      <div class="detail-scribble-yellow" id="detScribbleName" style="background:${ch.color};border:1px solid ${isLnk ? '#00e676' : 'transparent'};">${nameTxt}</div>
      <canvas class="detail-pan-trapezoid" id="detPanTrapezoid"></canvas>
      <div class="detail-ch-num-row">
        <span class="detail-ch-num">${String(ch.index + 1).padStart(2, '0')}${isLnk ? '⚯' : ''}</span>
        <span style="font-size:9px;color:#b0bec5;" id="detLeftDb">${formatDbText(ch.faderDb)}</span>
      </div>
      <div class="ch-fader-meter-zone">
        <canvas class="ch-fader-canvas" id="detLeftFaderCanvas"></canvas>
        <canvas class="ch-meter-canvas" id="detLeftMeterCanvas"></canvas>
      </div>
      <div class="detail-prev-next-row">
        <button class="cbtn" id="btnPrevCh">&lt;</button>
        <button class="cbtn" id="btnNextCh">&gt;</button>
      </div>
    `;
    body.appendChild(leftStrip);

    // Wire Left Strip interactions
    setTimeout(() => {
      const trapC = document.getElementById('detPanTrapezoid');
      const fadC = document.getElementById('detLeftFaderCanvas');
      const metC = document.getElementById('detLeftMeterCanvas');
      const dbEl = document.getElementById('detLeftDb');

      drawTrapezoidPan(trapC, ch.pan);
      drawFaderCanvas(fadC, ch.faderDb, false);
      drawLedMeterCanvas(metC, ch.peakDb, ch.holdDb, ch.peakDb, ch.holdDb, false, ch.clip);

      const onPanDrag = (c) => {
        ch.pan = Math.max(-1, Math.min(1, c.x * 2 - 1));
        if (isChannelLinked(ch.index)) {
          const oIdx = getLinkedChannelIndex(ch.index);
          state.channels[oIdx].pan = -ch.pan;
          syncChannelAudioParams(state.channels[oIdx]);
        }
        drawTrapezoidPan(trapC, ch.pan);
        syncChannelAudioParams(ch);
      };
      bindDrag(trapC, onPanDrag, onPanDrag, null, () => {
        ch.pan = isChannelLinked(ch.index) ? (ch.index % 2 === 0 ? -1.0 : 1.0) : 0;
        if (isChannelLinked(ch.index)) {
          const oIdx = getLinkedChannelIndex(ch.index);
          state.channels[oIdx].pan = -ch.pan;
          syncChannelAudioParams(state.channels[oIdx]);
        }
        drawTrapezoidPan(trapC, ch.pan);
        syncChannelAudioParams(ch);
      });

      const onFadDrag = (c) => {
        ch.faderDb = faderPosToDb(1.0 - c.y);
        if (isChannelLinked(ch.index)) {
          const oIdx = getLinkedChannelIndex(ch.index);
          state.channels[oIdx].faderDb = ch.faderDb;
        }
        if (dbEl) dbEl.textContent = formatDbText(ch.faderDb);
        drawFaderCanvas(fadC, ch.faderDb, false);
        syncSoloAndMuteRouting();
      };
      bindDrag(fadC, onFadDrag, onFadDrag, null, () => {
        ch.faderDb = 0;
        if (isChannelLinked(ch.index)) {
          const oIdx = getLinkedChannelIndex(ch.index);
          state.channels[oIdx].faderDb = 0;
        }
        if (dbEl) dbEl.textContent = formatDbText(0);
        drawFaderCanvas(fadC, 0, false);
        syncSoloAndMuteRouting();
      });

      const prevBtn = document.getElementById('btnPrevCh');
      const nextBtn = document.getElementById('btnNextCh');
      if (prevBtn) prevBtn.onclick = () => {
        state.selectedChannel = (state.selectedChannel + kMaxTracks - 1) % kMaxTracks;
        state.currentBank = Math.floor(state.selectedChannel / 8);
        syncSoloAndMuteRouting();
        renderDetailScreen();
      };
      if (nextBtn) nextBtn.onclick = () => {
        state.selectedChannel = (state.selectedChannel + 1) % kMaxTracks;
        state.currentBank = Math.floor(state.selectedChannel / 8);
        syncSoloAndMuteRouting();
        renderDetailScreen();
      };
    }, 0);

    // Right Main Area
    const rightMain = document.createElement('div');
    rightMain.className = 'detail-right-main';

    const subTabNames = ['CONFIG/PREAMP', 'GATE', 'DYN', 'EQ', 'SENDS', 'NAMING', 'PRESET'];
    const subTabsRow = document.createElement('div');
    subTabsRow.className = 'detail-subtabs-row';
    subTabNames.forEach((tName, idx) => {
      const b = document.createElement('button');
      b.className = `detail-subtab-btn ${state.detailSubTab === idx ? 'active' : ''}`;
      let label = tName;
      if (idx === 1 && ch.gateOn) label += ' ●';
      if (idx === 2 && ch.compOn) label += ' ●';
      if (idx === 3 && ch.eqOn) label += ' ●';
      b.textContent = label;
      b.onclick = () => {
        state.detailSubTab = idx;
        renderDetailScreen();
      };
      subTabsRow.appendChild(b);
    });
    rightMain.appendChild(subTabsRow);

    const card = document.createElement('div');
    card.className = 'detail-card-frame';
    rightMain.appendChild(card);
    body.appendChild(rightMain);
    screen.appendChild(body);

    if (state.detailSubTab === 0) renderDetailConfigTab(card, ch);
    else if (state.detailSubTab === 1) renderDetailGateTab(card, ch);
    else if (state.detailSubTab === 2) renderDetailDynTab(card, ch);
    else if (state.detailSubTab === 3) renderDetailEqTab(card, ch);
    else if (state.detailSubTab === 4) renderDetailSendsTab(card, ch);
    else if (state.detailSubTab === 5) renderDetailNamingTab(card, ch);
    else if (state.detailSubTab === 6) renderDetailPresetTab(card, ch);
  }

  // --- Detail Tab 0: CONFIG/PREAMP ---
  function renderDetailConfigTab(card, ch) {
    card.style.display = 'grid';
    card.style.gridTemplateColumns = '1.8fr 1fr 110px';
    card.style.gridTemplateRows = '68px 1fr 90px';
    card.style.gap = '6px';
    card.style.minHeight = '0';
    card.style.height = '100%';
    card.style.overflow = 'hidden';

    // 1. Signal Flow Box (Row 1, Col 1-2)
    const flowBox = document.createElement('div');
    flowBox.className = 'detail-sub-box';
    flowBox.style.gridColumn = '1 / 3';
    flowBox.style.minHeight = '0';
    flowBox.innerHTML = `
      <div class="detail-box-title">SIGNAL FLOW & INPUT SOURCE</div>
      <div style="flex:1;display:flex;align-items:center;gap:10px;min-height:0;">
        <button class="cbtn active-cyan" style="width:68px;height:24px;font-size:9.5px;">IN ${String(ch.index + 1).padStart(2, '0')}</button>
        <canvas id="sigFlowCanvas" style="flex:1;height:32px;"></canvas>
      </div>
    `;
    card.appendChild(flowBox);

    // 2. Preamp / Delay / Low Cut Box (Row 2, Col 1)
    const preBox = document.createElement('div');
    preBox.className = 'detail-sub-box';
    preBox.style.minHeight = '0';
    preBox.innerHTML = `<div class="detail-box-title">PREAMP / DELAY / LOW CUT</div>`;
    const preInner = document.createElement('div');
    preInner.style.cssText = 'flex:1;display:flex;align-items:center;justify-content:space-around;gap:4px;min-height:0;';

    const isLnk = isChannelLinked(ch.index);
    const leftBtns = document.createElement('div');
    leftBtns.style.cssText = 'display:flex;flex-direction:column;gap:3px;width:62px;';
    leftBtns.innerHTML = `
      <button class="cbtn ${isLnk ? 'active-green' : ''}" id="btnCfgLink" style="height:22px;font-size:8.5px;" title="Stereo Link adjacent channel">
        ${isLnk ? 'LINKED' : 'LINK'}
      </button>
      <button class="cbtn ${ch.phaseInvert ? 'active-yellow' : ''}" id="btnCfgPhase" style="height:22px;font-size:9px;">PHASE Ø</button>
      <button class="cbtn ${ch.phantom48v ? 'active-red' : ''}" id="btnCfg48v" style="height:22px;font-size:9px;">48V</button>
    `;
    preInner.appendChild(leftBtns);

    // Gain Dial
    preInner.appendChild(createConsoleDialElement({
      title: 'GAIN', min: -12, max: 60, def: 0, value: ch.trimDb, size: 60,
      format: v => (v >= 0 ? '+' : '') + v.toFixed(1) + ' dB',
      onChange: v => {
        ch.trimDb = v;
        syncLinkedChannel(ch, o => { o.trimDb = v; });
        syncChannelAudioParams(ch);
      }
    }));

    // Delay Dial + Toggle
    const dlyCol = document.createElement('div');
    dlyCol.style.cssText = 'display:flex;flex-direction:column;align-items:center;gap:2px;';
    dlyCol.appendChild(createConsoleDialElement({
      title: 'DELAY', min: 0.3, max: 500, def: 0.3, value: ch.delayMs, size: 52,
      format: v => v.toFixed(1) + 'ms',
      onChange: v => {
        ch.delayMs = v;
        syncLinkedChannel(ch, o => { o.delayMs = v; });
        syncChannelAudioParams(ch);
      }
    }));
    const dlyBtn = document.createElement('button');
    dlyBtn.className = `cbtn ${ch.delayOn ? 'active-yellow' : ''}`;
    dlyBtn.style.cssText = 'width:54px;height:18px;font-size:8.5px;';
    dlyBtn.textContent = 'DELAY';
    dlyBtn.onclick = () => {
      ch.delayOn = !ch.delayOn;
      syncLinkedChannel(ch, o => { o.delayOn = ch.delayOn; });
      syncChannelAudioParams(ch);
      renderDetailScreen();
    };
    dlyCol.appendChild(dlyBtn);
    preInner.appendChild(dlyCol);

    // Low Cut Dial + Toggle
    const lcCol = document.createElement('div');
    lcCol.style.cssText = 'display:flex;flex-direction:column;align-items:center;gap:2px;';
    lcCol.appendChild(createConsoleDialElement({
      title: 'LOW CUT', min: 20, max: 400, def: 80, isLog: true, value: ch.hpfFreq, size: 52,
      format: v => Math.round(v) + 'Hz',
      onChange: v => {
        ch.hpfFreq = v;
        syncLinkedChannel(ch, o => { o.hpfFreq = v; });
        syncChannelAudioParams(ch);
      }
    }));
    const lcBtn = document.createElement('button');
    lcBtn.className = `cbtn ${ch.hpfOn ? 'active-yellow' : ''}`;
    lcBtn.style.cssText = 'width:54px;height:18px;font-size:8.5px;';
    lcBtn.textContent = 'LOW CUT';
    lcBtn.onclick = () => {
      ch.hpfOn = !ch.hpfOn;
      syncLinkedChannel(ch, o => { o.hpfOn = ch.hpfOn; });
      syncChannelAudioParams(ch);
      renderDetailScreen();
    };
    lcCol.appendChild(lcBtn);
    preInner.appendChild(lcCol);

    preBox.appendChild(preInner);
    card.appendChild(preBox);

    // 3. Insert / Utils Box (Row 2, Col 2)
    const utilBox = document.createElement('div');
    utilBox.className = 'detail-sub-box';
    utilBox.innerHTML = `
      <div class="detail-box-title">INSERT / UTILS</div>
      <div style="flex:1;display:flex;flex-direction:column;justify-content:center;gap:6px;">
        <button class="cbtn" style="height:26px;" onclick="MPS.showScreen(3)">FX RACK (1-4)</button>
        <button class="cbtn" style="height:26px;" id="btnCopyCh">COPY CH</button>
        <button class="cbtn" style="height:26px;" id="btnPasteCh">PASTE CH</button>
      </div>
    `;
    card.appendChild(utilBox);

    // 4. DCA & Mute Groups Box (Row 3, Col 1-2)
    const grpBox = document.createElement('div');
    grpBox.className = 'detail-sub-box';
    grpBox.style.gridColumn = '1 / 3';
    grpBox.innerHTML = `
      <div style="display:flex;gap:12px;height:100%;">
        <div style="flex:1.2;display:flex;flex-direction:column;">
          <div class="detail-box-title">DCA GROUPS (1-8)</div>
          <div style="flex:1;display:grid;grid-template-columns:repeat(8,1fr);gap:3px;" id="cfgDcaGrid"></div>
        </div>
        <div style="flex:1;display:flex;flex-direction:column;">
          <div class="detail-box-title">MUTE GROUPS (1-6)</div>
          <div style="flex:1;display:grid;grid-template-columns:repeat(6,1fr);gap:3px;" id="cfgMgGrid"></div>
        </div>
      </div>
    `;
    card.appendChild(grpBox);

    // 5. Main Bus Box (Row 1-3, Col 3)
    const mainBox = document.createElement('div');
    mainBox.className = 'detail-sub-box';
    mainBox.style.gridRow = '1 / 4';
    mainBox.style.gridColumn = '3 / 4';
    mainBox.innerHTML = `
      <div style="height:2px;background:#ffa000;margin-bottom:3px;"></div>
      <div class="detail-box-title" style="text-align:center;color:#fff;">MAIN BUS</div>
      <button class="cbtn active-yellow" style="height:22px;margin-bottom:4px;">MAIN LR</button>
      <div style="flex:1;display:flex;justify-content:center;">
        <canvas id="cfgMainFader" style="width:48px;height:100%;"></canvas>
      </div>
      <div style="display:grid;grid-template-columns:1fr 1fr;gap:3px;height:22px;margin-top:4px;">
        <button class="cbtn ${ch.mute ? 'active-red' : ''}" id="cfgChMute">M</button>
        <button class="cbtn ${ch.solo ? 'active-yellow' : ''}" id="cfgChSolo">S</button>
      </div>
    `;
    card.appendChild(mainBox);

    setTimeout(() => {
      // Draw Signal Flow Diagram
      const sfC = document.getElementById('sigFlowCanvas');
      if (sfC) {
        const dpr = window.devicePixelRatio || 1;
        const w = sfC.clientWidth || 450;
        const h = sfC.clientHeight || 36;
        sfC.width = w * dpr; sfC.height = h * dpr;
        const g = sfC.getContext('2d');
        g.scale(dpr, dpr);
        const midY = h * 0.5;
        g.strokeStyle = '#9aa4b2'; g.lineWidth = 1.5;
        g.beginPath(); g.moveTo(8, midY); g.lineTo(w - 12, midY); g.stroke();
        const blocks = ['GAIN', 'Δt', 'HPF', 'GATE', 'EQ', 'DYN', 'OUT'];
        const step = (w - 40) / blocks.length;
        blocks.forEach((b, i) => {
          const bx = 20 + i * step;
          g.fillStyle = '#1b1f26';
          g.strokeStyle = '#00d2ff';
          g.fillRect(bx, midY - 10, 38, 20);
          g.strokeRect(bx, midY - 10, 38, 20);
          g.fillStyle = '#e5eaf2';
          g.font = 'bold 8.5px Inter, sans-serif';
          g.textAlign = 'center';
          g.fillText(b, bx + 19, midY + 3);
        });
      }

      document.getElementById('btnCfgLink').onclick = () => { toggleChannelLink(ch.index); renderDetailScreen(); };
      document.getElementById('btnCfgPhase').onclick = () => { ch.phaseInvert = !ch.phaseInvert; syncChannelAudioParams(ch); renderDetailScreen(); };
      document.getElementById('btnCfg48v').onclick = () => { ch.phantom48v = !ch.phantom48v; renderDetailScreen(); };
      document.getElementById('btnCopyCh').onclick = () => {
        state.clipboardCh = JSON.parse(JSON.stringify({
          trimDb: ch.trimDb, hpfOn: ch.hpfOn, hpfFreq: ch.hpfFreq,
          eqOn: ch.eqOn,
          eqLowType: ch.eqLowType || 'lowshelf', eqLowFreq: ch.eqLowFreq, eqLowGainDb: ch.eqLowGainDb, eqLowQ: ch.eqLowQ !== undefined ? ch.eqLowQ : 1.0,
          eqMid1Type: ch.eqMid1Type || 'peq', eqMid1Freq: ch.eqMid1Freq, eqMid1GainDb: ch.eqMid1GainDb, eqMid1Q: ch.eqMid1Q !== undefined ? ch.eqMid1Q : 1.4,
          eqMid2Type: ch.eqMid2Type || 'peq', eqMid2Freq: ch.eqMid2Freq, eqMid2GainDb: ch.eqMid2GainDb, eqMid2Q: ch.eqMid2Q !== undefined ? ch.eqMid2Q : 1.4,
          eqHighType: ch.eqHighType || 'highshelf', eqHighFreq: ch.eqHighFreq, eqHighGainDb: ch.eqHighGainDb, eqHighQ: ch.eqHighQ !== undefined ? ch.eqHighQ : 1.0,
          compOn: ch.compOn, compThreshDb: ch.compThreshDb, compRatio: ch.compRatio, compMakeupDb: ch.compMakeupDb
        }));
        showToast(`คัดลอกการตั้งค่าจาก ${ch.name} แล้ว`);
      };
      document.getElementById('btnPasteCh').onclick = () => {
        if (!state.clipboardCh) return showToast('ยังไม่มีข้อมูลที่คัดลอกไว้');
        Object.assign(ch, state.clipboardCh);
        syncChannelAudioParams(ch);
        renderDetailScreen();
        showToast(`วางการตั้งค่าลงใน ${ch.name} เรียบร้อย`);
      };

      const dcaGrid = document.getElementById('cfgDcaGrid');
      for (let d = 0; d < 8; d++) {
        const b = document.createElement('button');
        const on = (ch.dcaMask & (1 << d)) !== 0;
        b.className = `cbtn ${on ? 'active-cyan' : ''}`;
        b.textContent = String(d + 1);
        b.onclick = () => { ch.dcaMask ^= (1 << d); syncSoloAndMuteRouting(); renderDetailScreen(); };
        dcaGrid.appendChild(b);
      }

      const mgGrid = document.getElementById('cfgMgGrid');
      for (let m = 0; m < 6; m++) {
        const b = document.createElement('button');
        const on = (ch.muteGroupMask & (1 << m)) !== 0;
        b.className = `cbtn ${on ? 'active-red' : ''}`;
        b.textContent = String(m + 1);
        b.onclick = () => { ch.muteGroupMask ^= (1 << m); syncSoloAndMuteRouting(); renderDetailScreen(); };
        mgGrid.appendChild(b);
      }

      const mfC = document.getElementById('cfgMainFader');
      drawFaderCanvas(mfC, state.main.faderDb, false);
      const onMfDrag = (c) => {
        state.main.faderDb = faderPosToDb(1.0 - c.y);
        drawFaderCanvas(mfC, state.main.faderDb, false);
        syncSoloAndMuteRouting();
      };
      bindDrag(mfC, onMfDrag, onMfDrag);

      document.getElementById('cfgChMute').onclick = () => {
        ch.mute = !ch.mute;
        if (isChannelLinked(ch.index)) {
          const oIdx = getLinkedChannelIndex(ch.index);
          state.channels[oIdx].mute = ch.mute;
        }
        syncSoloAndMuteRouting();
        renderDetailScreen();
      };
      document.getElementById('cfgChSolo').onclick = () => {
        ch.solo = !ch.solo;
        if (isChannelLinked(ch.index)) {
          const oIdx = getLinkedChannelIndex(ch.index);
          state.channels[oIdx].solo = ch.solo;
        }
        syncSoloAndMuteRouting();
        renderDetailScreen();
      };
    }, 0);
  }

  // --- Detail Tab 1 & 2: GATE & DYN (Matches DetailScreen.cpp + TransferCurveGraph.cpp) ---
  function renderDetailGateTab(card, ch) {
    renderDynamicsLayout(card, ch, true);
  }

  function renderDetailDynTab(card, ch) {
    renderDynamicsLayout(card, ch, false);
  }

  function renderDynamicsLayout(card, ch, isGate) {
    card.style.display = 'grid';
    card.style.gridTemplateColumns = '300px 1fr';
    card.style.gap = '6px';
    card.style.minHeight = '0';
    card.style.height = '100%';
    card.style.overflow = 'hidden';

    const leftCol = document.createElement('div');
    leftCol.style.cssText = 'display:grid;grid-template-rows:repeat(3, 1fr);gap:4px;min-height:0;height:100%;min-width:0;overflow:hidden;';

    const box1 = document.createElement('div');
    box1.className = 'detail-sub-box';
    box1.style.cssText = 'min-height:0;overflow:hidden;padding:3px 6px;display:flex;flex-direction:column;';
    box1.innerHTML = `<div class="detail-box-title">${isGate ? 'THRESHOLD & RANGE' : 'THRESHOLD, RATIO & GAIN'}</div>`;
    const row1 = document.createElement('div');
    row1.style.cssText = 'flex:1;display:flex;align-items:center;justify-content:space-around;min-height:0;';

    const box2 = document.createElement('div');
    box2.className = 'detail-sub-box';
    box2.style.cssText = 'min-height:0;overflow:hidden;padding:3px 6px;display:flex;flex-direction:column;';
    box2.innerHTML = `<div class="detail-box-title">${isGate ? 'ENVELOPE DYNAMICS' : 'ENVELOPE & PARALLEL MIX'}</div>`;
    const row2 = document.createElement('div');
    row2.style.cssText = 'flex:1;display:flex;align-items:center;justify-content:space-around;min-height:0;';

    const box3 = document.createElement('div');
    box3.className = 'detail-sub-box';
    box3.style.cssText = 'min-height:0;overflow:hidden;padding:3px 6px;display:flex;flex-direction:column;';
    box3.innerHTML = `<div class="detail-box-title">SIDECHAIN FILTER</div>`;
    const row3 = document.createElement('div');
    row3.style.cssText = 'flex:1;display:flex;align-items:center;justify-content:space-around;min-height:0;';

    function updateDynToggleUi() {
      const btn = document.getElementById('btnDynToggle');
      const statusLbl = document.getElementById('dynStatusLabel');
      const curOn = isGate ? ch.gateOn : ch.compOn;
      if (btn) {
        btn.className = `cbtn ${curOn ? 'active-yellow' : ''}`;
        btn.textContent = isGate ? (curOn ? 'GATE IN' : 'GATE BYP') : (curOn ? 'DYN IN' : 'DYN BYP');
      }
      if (statusLbl) {
        statusLbl.textContent = isGate
          ? (curOn ? 'EXPANDER / NOISE GATE (ACTIVE)' : 'EXPANDER / NOISE GATE (BYPASS)')
          : (curOn ? 'SOFT-KNEE COMPRESSOR (ACTIVE)' : 'SOFT-KNEE COMPRESSOR (BYPASS)');
        statusLbl.style.color = curOn ? '#ffb74d' : '#8b949e';
      }
      const subTabs = document.querySelectorAll('.detail-subtab-btn');
      if (subTabs.length > (isGate ? 1 : 2)) {
        const tabIdx = isGate ? 1 : 2;
        const baseName = isGate ? 'GATE' : 'DYN';
        subTabs[tabIdx].textContent = curOn ? `${baseName} ●` : baseName;
      }
    }

    if (isGate) {
      row1.appendChild(createConsoleDialElement({
        title: 'THRESH', min: -80, max: 0, def: -45, value: ch.gateThreshDb, size: 54,
        format: v => v.toFixed(0) + ' dB',
        onChange: v => {
          ch.gateThreshDb = v;
          ch.gateOn = true;
          syncLinkedChannel(ch, o => { o.gateThreshDb = v; o.gateOn = true; });
          syncChannelAudioParams(ch);
          updateDynToggleUi();
          drawTransferCurve();
        }
      }));
      row1.appendChild(createConsoleDialElement({
        title: 'RANGE', min: 3, max: 60, def: 40, value: ch.gateRangeDb, size: 54,
        format: v => v.toFixed(0) + ' dB',
        onChange: v => {
          ch.gateRangeDb = v;
          ch.gateOn = true;
          syncLinkedChannel(ch, o => { o.gateRangeDb = v; o.gateOn = true; });
          syncChannelAudioParams(ch);
          updateDynToggleUi();
          drawTransferCurve();
        }
      }));

      row2.appendChild(createConsoleDialElement({
        title: 'ATTACK', min: 0, max: 120, def: 10, value: ch.gateAttackMs, size: 50,
        format: v => v.toFixed(0) + 'ms',
        onChange: v => {
          ch.gateAttackMs = v; ch.gateOn = true;
          syncLinkedChannel(ch, o => { o.gateAttackMs = v; o.gateOn = true; });
          syncChannelAudioParams(ch);
          updateDynToggleUi();
        }
      }));
      row2.appendChild(createConsoleDialElement({
        title: 'HOLD', min: 0, max: 2000, def: 50, value: ch.gateHoldMs, size: 50,
        format: v => v >= 1000 ? (v / 1000).toFixed(1) + 's' : v.toFixed(0) + 'ms',
        onChange: v => {
          ch.gateHoldMs = v; ch.gateOn = true;
          syncLinkedChannel(ch, o => { o.gateHoldMs = v; o.gateOn = true; });
          syncChannelAudioParams(ch);
          updateDynToggleUi();
        }
      }));
      row2.appendChild(createConsoleDialElement({
        title: 'RELEASE', min: 5, max: 4000, def: 150, value: ch.gateReleaseMs, size: 50,
        format: v => v >= 1000 ? (v / 1000).toFixed(1) + 's' : v.toFixed(0) + 'ms',
        onChange: v => {
          ch.gateReleaseMs = v; ch.gateOn = true;
          syncLinkedChannel(ch, o => { o.gateReleaseMs = v; o.gateOn = true; });
          syncChannelAudioParams(ch);
          updateDynToggleUi();
        }
      }));
    } else {
      row1.appendChild(createConsoleDialElement({
        title: 'THRESH', min: -60, max: 0, def: -20, value: ch.compThreshDb, size: 46,
        format: v => v.toFixed(0) + ' dB',
        onChange: v => {
          ch.compThreshDb = v;
          ch.compOn = true;
          syncLinkedChannel(ch, o => { o.compThreshDb = v; o.compOn = true; });
          updateDynToggleUi();
          syncChannelAudioParams(ch);
          drawTransferCurve();
        }
      }));
      row1.appendChild(createConsoleDialElement({
        title: 'RATIO', min: 1.1, max: 20, def: 4, value: ch.compRatio, size: 46,
        format: v => v.toFixed(1) + ':1',
        onChange: v => {
          ch.compRatio = v;
          ch.compOn = true;
          syncLinkedChannel(ch, o => { o.compRatio = v; o.compOn = true; });
          updateDynToggleUi();
          syncChannelAudioParams(ch);
          drawTransferCurve();
        }
      }));
      row1.appendChild(createConsoleDialElement({
        title: 'GAIN', min: 0, max: 24, def: 0, value: ch.compMakeupDb, size: 46,
        format: v => '+' + v.toFixed(1) + 'dB',
        onChange: v => {
          ch.compMakeupDb = v;
          syncLinkedChannel(ch, o => { o.compMakeupDb = v; });
          syncChannelAudioParams(ch);
        }
      }));
      row1.appendChild(createConsoleDialElement({
        title: 'KNEE', min: 0, max: 10, def: 2, value: ch.compKneeDb, size: 46,
        format: v => v.toFixed(0),
        onChange: v => {
          ch.compKneeDb = v;
          syncLinkedChannel(ch, o => { o.compKneeDb = v; });
          syncChannelAudioParams(ch);
          drawTransferCurve();
        }
      }));

      row2.appendChild(createConsoleDialElement({
        title: 'ATTACK', min: 0, max: 120, def: 15, value: ch.compAttackMs, size: 46,
        format: v => v.toFixed(0) + 'ms',
        onChange: v => {
          ch.compAttackMs = v;
          syncLinkedChannel(ch, o => { o.compAttackMs = v; });
          syncChannelAudioParams(ch);
        }
      }));
      row2.appendChild(createConsoleDialElement({
        title: 'HOLD', min: 0, max: 1000, def: 10, value: ch.compHoldMs, size: 46,
        format: v => v.toFixed(0) + 'ms',
        onChange: v => {
          ch.compHoldMs = v;
          syncLinkedChannel(ch, o => { o.compHoldMs = v; });
        }
      }));
      row2.appendChild(createConsoleDialElement({
        title: 'RELEASE', min: 5, max: 2000, def: 150, value: ch.compReleaseMs, size: 46,
        format: v => v.toFixed(0) + 'ms',
        onChange: v => {
          ch.compReleaseMs = v;
          syncLinkedChannel(ch, o => { o.compReleaseMs = v; });
          syncChannelAudioParams(ch);
        }
      }));
      row2.appendChild(createConsoleDialElement({
        title: 'MIX', min: 0, max: 100, def: 100, value: ch.compMixPct, size: 46,
        format: v => v.toFixed(0) + '%',
        onChange: v => {
          ch.compMixPct = v;
          syncLinkedChannel(ch, o => { o.compMixPct = v; });
          syncChannelAudioParams(ch);
        }
      }));
    }

    row3.appendChild(createConsoleDialElement({
      title: 'FREQ', min: 20, max: 20000, def: 250, isLog: true, value: ch.gateFiltFreq, size: 50,
      format: v => v >= 1000 ? (v / 1000).toFixed(1) + 'k' : Math.round(v) + 'Hz',
      onChange: v => {
        ch.gateFiltFreq = v;
        syncLinkedChannel(ch, o => { o.gateFiltFreq = v; });
      }
    }));
    const scBtns = document.createElement('div');
    scBtns.style.cssText = 'display:flex;gap:4px;align-items:center;';
    scBtns.innerHTML = `<button class="cbtn" style="width:54px;height:22px;font-size:9px;">FILTER</button><button class="cbtn" style="width:48px;height:22px;font-size:9px;">SELF</button>`;
    row3.appendChild(scBtns);

    box1.appendChild(row1);
    box2.appendChild(row2);
    box3.appendChild(row3);
    leftCol.appendChild(box1);
    leftCol.appendChild(box2);
    leftCol.appendChild(box3);
    card.appendChild(leftCol);

    // Right Monitor / Transfer Curve Box
    const rightBox = document.createElement('div');
    rightBox.className = 'detail-sub-box';
    rightBox.style.cssText = 'background:#161920;min-height:0;height:100%;min-width:0;display:flex;flex-direction:column;overflow:hidden;padding:4px 8px;';
    const isOn = isGate ? ch.gateOn : ch.compOn;
    rightBox.innerHTML = `
      <div class="detail-box-title" style="text-align:center;margin-bottom:2px;">TRANSFER CURVE / MONITOR</div>
      <div style="flex:1;display:flex;gap:6px;min-height:0;min-width:0;overflow:hidden;">
        <canvas id="transferCurveCanvas" style="flex:1;min-width:0;min-height:0;width:100%;height:100%;display:block;"></canvas>
        <div style="width:28px;background:#0a0c10;border:1px solid #262c36;border-radius:3px;display:flex;flex-direction:column;align-items:center;padding:2px;flex-shrink:0;">
          <span style="font-size:7.5px;color:#ffb74d;font-weight:700;">GR</span>
          <div style="flex:1;width:10px;background:#14181f;margin-top:2px;position:relative;overflow:hidden;border-radius:1px;">
            <div id="grBarFill" style="position:absolute;top:0;left:0;right:0;height:0%;background:linear-gradient(180deg,#ff1744,#ff9100);"></div>
          </div>
          <span id="grNumVal" style="font-size:7px;color:#ff9100;margin-top:2px;font-family:monospace;">0dB</span>
        </div>
      </div>
      <div style="height:24px;display:flex;align-items:center;justify-content:space-between;margin-top:4px;flex-shrink:0;">
        <button class="cbtn ${isOn ? 'active-yellow' : ''}" id="btnDynToggle" style="width:92px;height:22px;font-size:9.5px;">
          ${isGate ? (isOn ? 'GATE IN' : 'GATE BYP') : (isOn ? 'DYN IN' : 'DYN BYP')}
        </button>
        <span id="dynStatusLabel" style="font-size:9.5px;color:${isOn ? '#ffb74d' : '#8b949e'};font-weight:600;">
          ${isGate ? (isOn ? 'EXPANDER / NOISE GATE (ACTIVE)' : 'EXPANDER / NOISE GATE (BYPASS)') : (isOn ? 'SOFT-KNEE COMPRESSOR (ACTIVE)' : 'SOFT-KNEE COMPRESSOR (BYPASS)')}
        </span>
      </div>
    `;
    card.appendChild(rightBox);

    function drawTransferCurve() {
      const c = document.getElementById('transferCurveCanvas');
      if (!c) return;
      const dpr = window.devicePixelRatio || 1;
      const rect = c.getBoundingClientRect();
      const w = Math.floor(rect.width) || c.clientWidth || 300;
      const h = Math.floor(rect.height) || c.clientHeight || 180;
      if (c.width !== Math.round(w * dpr) || c.height !== Math.round(h * dpr)) {
        c.width = Math.round(w * dpr);
        c.height = Math.round(h * dpr);
      }
      const g = c.getContext('2d');
      g.save();
      g.scale(dpr, dpr);

      g.fillStyle = '#0d0f12';
      g.fillRect(0, 0, w, h);
      g.strokeStyle = '#1a1e24';
      g.lineWidth = 1.0;
      for (let i = 1; i < 10; i++) {
        const gx = Math.round((w / 10) * i);
        const gy = Math.round((h / 10) * i);
        g.beginPath(); g.moveTo(gx, 0); g.lineTo(gx, h); g.stroke();
        g.beginPath(); g.moveTo(0, gy); g.lineTo(w, gy); g.stroke();
      }
      // 1:1 diagonal reference
      g.strokeStyle = '#282f3a';
      g.beginPath(); g.moveTo(0, h); g.lineTo(w, 0); g.stroke();

      const minDb = isGate ? -80 : -60;
      const span = -minDb;
      const threshVal = isGate ? ch.gateThreshDb : ch.compThreshDb;

      // Threshold line
      const thX = ((threshVal - minDb) / span) * w;
      if (thX > 0 && thX < w) {
        g.strokeStyle = 'rgba(255, 160, 0, 0.5)';
        g.setLineDash([3, 3]);
        g.beginPath(); g.moveTo(thX, 0); g.lineTo(thX, h); g.stroke();
        g.setLineDash([]);
        g.fillStyle = '#ffb74d';
        g.font = '8px monospace';
        g.fillText('T: ' + threshVal.toFixed(0) + 'dB', Math.max(4, Math.min(w - 45, thX + 3)), 12);
      }

      const active = isGate ? ch.gateOn : ch.compOn;
      g.strokeStyle = active ? '#00d2ff' : '#607d8b';
      g.lineWidth = 2.0;
      g.beginPath();
      for (let x = 0; x <= w; x += 2) {
        const inDb = minDb + (x / w) * span;
        let outDb = inDb;
        if (isGate) {
          if (inDb < ch.gateThreshDb) {
            outDb = Math.max(minDb, inDb - ch.gateRangeDb);
          }
        } else {
          const knee = Math.max(0, ch.compKneeDb);
          const th = ch.compThreshDb;
          const ratio = Math.max(1.0, ch.compRatio);
          const delta = inDb - th;
          if (knee > 0 && 2 * delta >= -knee && 2 * delta <= knee) {
            outDb = inDb + ((1 / ratio - 1) * Math.pow(delta + knee / 2, 2)) / (2 * knee);
          } else if (delta > (knee / 2)) {
            outDb = th + delta / ratio;
          }
        }
        const y = h - ((outDb - minDb) / span) * h;
        if (x === 0) g.moveTo(x, y); else g.lineTo(x, y);
      }
      g.stroke();

      if (!active) {
        g.fillStyle = 'rgba(255, 255, 255, 0.18)';
        g.font = 'bold 12px sans-serif';
        g.textAlign = 'center';
        g.fillText('BYPASS', w / 2, h / 2);
        g.textAlign = 'start';
      }

      g.restore();
    }

    setTimeout(() => {
      drawTransferCurve();
      const btn = document.getElementById('btnDynToggle');
      if (btn) {
        btn.onclick = () => {
          if (isGate) {
            ch.gateOn = !ch.gateOn;
            syncLinkedChannel(ch, o => { o.gateOn = ch.gateOn; });
          } else {
            ch.compOn = !ch.compOn;
            syncLinkedChannel(ch, o => { o.compOn = ch.compOn; });
          }
          syncChannelAudioParams(ch);
          updateDynToggleUi();
          drawTransferCurve();
        };
      }
    }, 0);
  }

  // --- EQ Filter Gain Formula for Arbitrary Filter Types ---
  function calcBandGainAt(type, f, fc, gainDb, q) {
    if (!type) type = 'peq';
    fc = Math.max(20, Math.min(20000, fc));
    if (type === 'peq' || type === 'peaking') {
      const oct = Math.log2(f / fc);
      const qVal = Math.max(0.2, q || 1.4);
      return gainDb * Math.exp(-oct * oct * 3.0 * qVal);
    }
    if (type === 'lowshelf') {
      return gainDb / (1 + Math.pow(f / fc, 2.0));
    }
    if (type === 'highshelf') {
      return gainDb / (1 + Math.pow(fc / f, 2.0));
    }
    if (type === 'lowcut' || type === 'highpass') {
      if (f < fc) {
        const oct = Math.log2(fc / f);
        return -12.0 * oct;
      } else {
        const oct = Math.log2(f / fc);
        const res = (Math.max(0.5, q || 0.707) - 0.707) * 4.0;
        return res > 0 ? res * Math.exp(-oct * oct * 8.0) : 0.0;
      }
    }
    if (type === 'hicut' || type === 'lowpass') {
      if (f > fc) {
        const oct = Math.log2(f / fc);
        return -12.0 * oct;
      } else {
        const oct = Math.log2(fc / f);
        const res = (Math.max(0.5, q || 0.707) - 0.707) * 4.0;
        return res > 0 ? res * Math.exp(-oct * oct * 8.0) : 0.0;
      }
    }
    return 0;
  }

  function setEqBandType(bIdx, newType) {
    const ch = state.channels[state.selectedChannel];
    if (!ch) return;
    if (bIdx === 1) ch.eqLowType = newType;
    else if (bIdx === 2) ch.eqMid1Type = newType;
    else if (bIdx === 3) ch.eqMid2Type = newType;
    else if (bIdx === 4) ch.eqHighType = newType;
    ch.eqOn = true;

    syncLinkedChannel(ch, o => {
      if (bIdx === 1) o.eqLowType = newType;
      else if (bIdx === 2) o.eqMid1Type = newType;
      else if (bIdx === 3) o.eqMid2Type = newType;
      else if (bIdx === 4) o.eqHighType = newType;
      o.eqOn = true;
    });

    syncChannelAudioParams(ch);
    triggerAutoSaveSession();
    renderDetailScreen();

    const typeNames = {
      peq: 'PEQ (Parametric Bell)',
      lowcut: 'Low Cut / HPF (Highpass)',
      hicut: 'Hi Cut / LPF (Lowpass)',
      lowshelf: 'Low Shelf (L-SHV)',
      highshelf: 'High Shelf (H-SHV)'
    };
    showToast(`เปลี่ยน Band ${bIdx} เป็น ${typeNames[newType] || newType}`);
  }

  // --- Detail Tab 3: 4-BAND PARAMETRIC EQ + RTA (Matches InteractiveEqGraph.cpp) ---
  function renderDetailEqTab(card, ch) {
    card.style.display = 'grid';
    card.style.gridTemplateColumns = '165px 1fr';
    card.style.gap = '6px';
    card.style.minHeight = '0';
    card.style.height = '100%';
    card.style.overflow = 'hidden';

    // Left Knobs Box
    const leftBox = document.createElement('div');
    leftBox.className = 'detail-sub-box';
    leftBox.style.cssText = 'min-height:0;overflow:hidden;padding:3px;display:flex;flex-direction:column;gap:2px;';
    const bIdx = state.selectedEqBand;
    const bandColors = ['#e53935', '#00d2ff', '#ffc107', '#e040fb', '#ff9800'];
    const curCol = bandColors[bIdx] || '#00d2ff';

    const curType = bIdx === 0 ? 'lowcut' : (bIdx === 1 ? (ch.eqLowType || 'lowshelf') : (bIdx === 2 ? (ch.eqMid1Type || 'peq') : (bIdx === 3 ? (ch.eqMid2Type || 'peq') : (ch.eqHighType || 'highshelf'))));

    const typeDisplayNames = {
      peq: 'PEQ',
      lowcut: 'LCUT',
      hicut: 'HCUT',
      lowshelf: 'L-SHV',
      highshelf: 'H-SHV'
    };
    const curTypeLabel = typeDisplayNames[curType] || curType.toUpperCase();
    const isCutType = curType === 'lowcut' || curType === 'hicut' || bIdx === 0;

    const getBandFreq = () => bIdx === 0 ? ch.hpfFreq : bIdx === 1 ? ch.eqLowFreq : bIdx === 2 ? ch.eqMid1Freq : bIdx === 3 ? ch.eqMid2Freq : ch.eqHighFreq;
    const getBandGain = () => bIdx === 0 ? 0 : bIdx === 1 ? ch.eqLowGainDb : bIdx === 2 ? ch.eqMid1GainDb : bIdx === 3 ? ch.eqMid2GainDb : ch.eqHighGainDb;
    const getBandQ = () => {
      if (bIdx === 0) return 0.707;
      if (bIdx === 1) return ch.eqLowQ !== undefined ? ch.eqLowQ : 1.0;
      if (bIdx === 2) return ch.eqMid1Q !== undefined ? ch.eqMid1Q : 1.4;
      if (bIdx === 3) return ch.eqMid2Q !== undefined ? ch.eqMid2Q : 1.4;
      if (bIdx === 4) return ch.eqHighQ !== undefined ? ch.eqHighQ : 1.0;
      return 1.0;
    };

    const bandInfoMap = [
      { name: 'LO CUT (HPF)', range: '20 - 400 Hz', desc: 'ตัดเสียงลม/เสียงหึ่งต่ำ (Rumble)', tag: 'Sub Cut' },
      { name: '1 LOW (Bass)', range: '20 - 250 Hz', desc: 'ย่านเบส & กระเดื่อง (ความหนักแน่น)', tag: 'Bass' },
      { name: '2 LOMID (Body)', range: '250 - 2 kHz', desc: 'ย่านกลางต่ำ (เนื้อเสียงร้อง, บอดี้สแนร์)', tag: 'Body / Mid' },
      { name: '3 HIMID (Presence)', range: '1.5 - 6 kHz', desc: 'ย่านกลางสูง (ความคมชัด, พุ่งเด่น)', tag: 'Presence' },
      { name: '4 HIGH (Air)', range: '4 - 20 kHz', desc: 'ย่านแหลม & ฉาบ (ประกายเสียง, แอร์)', tag: 'Air / Treble' }
    ];
    const bInfo = bandInfoMap[bIdx] || bandInfoMap[1];

    const curFreqVal = getBandFreq();
    const freqFormatted = curFreqVal >= 1000 ? (curFreqVal / 1000).toFixed(2) + ' kHz' : Math.round(curFreqVal) + ' Hz';

    const bandHeader = document.createElement('div');
    bandHeader.style.cssText = `background:#121620;border:1px solid ${curCol};border-radius:4px;padding:3px 5px;display:flex;flex-direction:column;gap:2px;flex-shrink:0;`;
    bandHeader.innerHTML = `
      <div style="display:flex;align-items:center;justify-content:space-between;">
        <span style="font-weight:800;color:${curCol};font-size:10.5px;">${bInfo.name}</span>
        <span id="eqActiveFreqPill" style="font-family:var(--font-mono);font-weight:700;color:#fff;font-size:10.5px;background:#000;padding:1px 4px;border-radius:3px;border:1px solid #334155;">${freqFormatted}</span>
      </div>
      <div style="font-size:8.5px;color:#94a3b8;font-weight:600;white-space:nowrap;overflow:hidden;text-overflow:ellipsis;">ช่วง: <b style="color:#e2e8f0;">${bInfo.range}</b> • ${bInfo.desc}</div>
      ${bIdx >= 1 ? `
      <div style="display:flex;align-items:center;justify-content:space-between;margin-top:1px;padding-top:2px;border-top:1px solid #1f2735;">
        <span style="font-size:8.5px;color:#94a3b8;font-weight:700;">TYPE:</span>
        <div style="display:flex;gap:2px;">
          <button class="cbtn ${curType === 'peq' ? 'active-cyan' : ''}" style="height:19px;padding:0 5px;font-size:8.5px;font-weight:700;" onclick="MPS.setEqBandType(${bIdx}, 'peq')" title="Parametric Bell (PEQ)">PEQ</button>
          <button class="cbtn ${curType === 'lowcut' ? 'active-red' : ''}" style="height:19px;padding:0 5px;font-size:8.5px;font-weight:700;" onclick="MPS.setEqBandType(${bIdx}, 'lowcut')" title="Low Cut / HPF">LCUT</button>
          <button class="cbtn ${curType === 'hicut' ? 'active-yellow' : ''}" style="height:19px;padding:0 5px;font-size:8.5px;font-weight:700;" onclick="MPS.setEqBandType(${bIdx}, 'hicut')" title="Hi Cut / LPF">HCUT</button>
          <button class="cbtn ${(curType === 'lowshelf' || curType === 'highshelf') ? 'active-cyan' : ''}" style="height:19px;padding:0 5px;font-size:8.5px;font-weight:700;" onclick="MPS.setEqBandType(${bIdx}, '${bIdx >= 3 ? 'highshelf' : 'lowshelf'}')" title="${bIdx >= 3 ? 'High Shelf' : 'Low Shelf'}">${bIdx >= 3 ? 'H-SHV' : 'L-SHV'}</button>
        </div>
      </div>
      ` : `
      <div style="display:flex;align-items:center;justify-content:space-between;margin-top:1px;padding-top:2px;border-top:1px solid #1f2735;">
        <span style="font-size:8.5px;color:#ff5252;font-weight:700;">TYPE: LOW CUT (HPF -12dB/oct)</span>
      </div>
      `}
    `;
    leftBox.appendChild(bandHeader);

    const dialsGrid = document.createElement('div');
    dialsGrid.style.cssText = 'flex:1;display:grid;grid-template-columns:1fr 1fr;align-items:center;justify-items:center;gap:2px;min-height:0;overflow:hidden;';

    dialsGrid.appendChild(createConsoleDialElement({
      title: 'GAIN', min: -15, max: 15, def: 0, value: isCutType ? 0 : getBandGain(), size: 44, color: isCutType ? '#64748b' : curCol,
      format: v => isCutType ? 'CUT' : ((v >= 0 ? '+' : '') + v.toFixed(1) + 'dB'),
      onChange: v => {
        if (isCutType) return;
        if (bIdx === 1) ch.eqLowGainDb = v;
        else if (bIdx === 2) ch.eqMid1GainDb = v;
        else if (bIdx === 3) ch.eqMid2GainDb = v;
        else if (bIdx === 4) ch.eqHighGainDb = v;
        ch.eqOn = true;
        syncLinkedChannel(ch, o => {
          if (bIdx === 1) o.eqLowGainDb = v;
          else if (bIdx === 2) o.eqMid1GainDb = v;
          else if (bIdx === 3) o.eqMid2GainDb = v;
          else if (bIdx === 4) o.eqHighGainDb = v;
          o.eqOn = true;
        });
        syncChannelAudioParams(ch);
      }
    }));

    dialsGrid.appendChild(createConsoleDialElement({
      title: 'FREQ', min: 20, max: 20000, def: 1000, isLog: true, value: getBandFreq(), size: 44, color: curCol,
      format: v => v >= 1000 ? (v / 1000).toFixed(1) + 'k' : Math.round(v) + 'Hz',
      onChange: v => {
        if (bIdx === 0) { ch.hpfFreq = Math.min(400, v); ch.hpfOn = true; }
        else if (bIdx === 1) ch.eqLowFreq = v;
        else if (bIdx === 2) ch.eqMid1Freq = v;
        else if (bIdx === 3) ch.eqMid2Freq = v;
        else if (bIdx === 4) ch.eqHighFreq = v;
        syncLinkedChannel(ch, o => {
          if (bIdx === 0) { o.hpfFreq = ch.hpfFreq; o.hpfOn = true; }
          else if (bIdx === 1) o.eqLowFreq = v;
          else if (bIdx === 2) o.eqMid1Freq = v;
          else if (bIdx === 3) o.eqMid2Freq = v;
          else if (bIdx === 4) o.eqHighFreq = v;
        });
        const curF = getBandFreq();
        const fStr = curF >= 1000 ? (curF / 1000).toFixed(2) + ' kHz' : Math.round(curF) + ' Hz';
        const pill = document.getElementById('eqActiveFreqPill');
        if (pill) pill.textContent = fStr;
        syncChannelAudioParams(ch);
      }
    }));

    dialsGrid.appendChild(createConsoleDialElement({
      title: 'Q', min: 0.3, max: 10.0, def: isCutType ? 0.707 : 1.4, isLog: true, value: getBandQ(), size: 44, color: curCol,
      format: v => v.toFixed(2),
      onChange: v => {
        if (bIdx === 1) ch.eqLowQ = v;
        else if (bIdx === 2) ch.eqMid1Q = v;
        else if (bIdx === 3) ch.eqMid2Q = v;
        else if (bIdx === 4) ch.eqHighQ = v;
        syncLinkedChannel(ch, o => {
          if (bIdx === 1) o.eqLowQ = v;
          else if (bIdx === 2) o.eqMid1Q = v;
          else if (bIdx === 3) o.eqMid2Q = v;
          else if (bIdx === 4) o.eqHighQ = v;
        });
        syncChannelAudioParams(ch);
      }
    }));

    const eqBtnsCol = document.createElement('div');
    eqBtnsCol.style.cssText = 'display:flex;flex-direction:column;gap:4px;width:68px;justify-content:center;';
    eqBtnsCol.innerHTML = `
      <button class="cbtn ${ch.eqOn ? 'active-cyan' : ''}" id="btnEqIn" style="height:24px;font-size:10px;">${ch.eqOn ? 'EQ IN' : 'EQ BYP'}</button>
      <button class="cbtn ${ch.hpfOn ? 'active-yellow' : ''}" id="btnEqLocut" style="height:24px;font-size:10px;">LO CUT</button>
    `;
    dialsGrid.appendChild(eqBtnsCol);
    leftBox.appendChild(dialsGrid);
    card.appendChild(leftBox);

    // Right Interactive EQ Graph + Bottom Band Selector Bar
    const rightCol = document.createElement('div');
    rightCol.style.cssText = 'display:flex;flex-direction:column;gap:3px;min-width:0;min-height:0;height:100%;overflow:hidden;';
    rightCol.innerHTML = `
      <div style="flex:1;position:relative;min-height:0;overflow:hidden;">
        <canvas id="interactiveEqCanvas" style="width:100%;height:100%;display:block;cursor:crosshair;"></canvas>
      </div>
      <div style="height:24px;flex-shrink:0;display:flex;align-items:center;justify-content:space-between;gap:4px;">
        <div style="display:flex;gap:3px;align-items:center;">
          <button class="cbtn" id="btnEqReset" style="width:46px;height:22px;font-size:9.5px;" title="รีเซ็ตค่า EQ เป็นค่าเริ่มต้น">RESET</button>
          <button class="cbtn active-cyan" id="btnEqTypeSelect" style="min-width:64px;height:22px;font-size:9.5px;font-weight:800;background:#003847;border:1px solid #00d2ff;color:#00e5ff;" title="เปลี่ยนรูปแบบ EQ Filter">${curTypeLabel} ▼</button>
          <button class="cbtn ${state.rtaEnabled ? 'active-yellow' : ''}" id="btnEqRta" style="width:40px;height:22px;font-size:9.5px;">RTA</button>
          <button class="cbtn ${bIdx === 0 ? 'active-red' : ''}" id="btnBandHpf" style="width:50px;height:22px;font-size:9.5px;">LO CUT</button>
        </div>
        <div style="display:flex;gap:3px;">
          <button class="cbtn ${bIdx === 1 ? 'active-cyan' : ''}" onclick="MPS.selectEqBand(1)" style="min-width:52px;height:22px;font-size:9.5px;">1 LOW</button>
          <button class="cbtn ${bIdx === 2 ? 'active-yellow' : ''}" onclick="MPS.selectEqBand(2)" style="min-width:56px;height:22px;font-size:9.5px;">2 LOMID</button>
          <button class="cbtn ${bIdx === 3 ? 'active-cyan' : ''}" onclick="MPS.selectEqBand(3)" style="min-width:56px;height:22px;font-size:9.5px;">3 HIMID</button>
          <button class="cbtn ${bIdx === 4 ? 'active-yellow' : ''}" onclick="MPS.selectEqBand(4)" style="min-width:52px;height:22px;font-size:9.5px;">4 HIGH</button>
        </div>
      </div>
    `;
    card.appendChild(rightCol);

    setTimeout(() => {
      document.getElementById('btnEqIn').onclick = () => {
        ch.eqOn = !ch.eqOn;
        syncLinkedChannel(ch, o => { o.eqOn = ch.eqOn; });
        syncChannelAudioParams(ch);
        renderDetailScreen();
      };
      document.getElementById('btnEqLocut').onclick = () => {
        ch.hpfOn = !ch.hpfOn;
        syncLinkedChannel(ch, o => { o.hpfOn = ch.hpfOn; });
        syncChannelAudioParams(ch);
        renderDetailScreen();
      };
      document.getElementById('btnBandHpf').onclick = () => { state.selectedEqBand = 0; renderDetailScreen(); };
      document.getElementById('btnEqRta').onclick = () => { state.rtaEnabled = !state.rtaEnabled; renderDetailScreen(); };

      const btnTypeSel = document.getElementById('btnEqTypeSelect');
      if (btnTypeSel) {
        btnTypeSel.onclick = (e) => {
          e.stopPropagation();
          if (bIdx === 0) {
            showToast('Band 0 เป็น Preamp Lo Cut (HPF)');
            return;
          }
          openPopupMenu([
            { header: `FILTER TYPE (BAND ${bIdx})` },
            { label: '🔔 PEQ (Parametric Bell - บูสต์/คัทแบบเบลล์)', checked: curType === 'peq', action: () => setEqBandType(bIdx, 'peq') },
            { label: '📉 Low Cut / HPF (ตัดย่านความถี่ต่ำออก)', checked: curType === 'lowcut', action: () => setEqBandType(bIdx, 'lowcut') },
            { label: '📈 Hi Cut / LPF (ตัดย่านความถี่สูงออก)', checked: curType === 'hicut', action: () => setEqBandType(bIdx, 'hicut') },
            { label: '🎚️ Low Shelf (L-SHV - เพิ่ม/ลดย่านต่ำยกแผง)', checked: curType === 'lowshelf', action: () => setEqBandType(bIdx, 'lowshelf') },
            { label: '🎚️ High Shelf (H-SHV - เพิ่ม/ลดย่านสูงยกแผง)', checked: curType === 'highshelf', action: () => setEqBandType(bIdx, 'highshelf') }
          ], 180, state.stageH - 120);
        };
      }

      document.getElementById('btnEqReset').onclick = () => {
        ch.eqLowGainDb = 0; ch.eqMid1GainDb = 0; ch.eqMid2GainDb = 0; ch.eqHighGainDb = 0;
        ch.eqLowType = 'lowshelf'; ch.eqMid1Type = 'peq'; ch.eqMid2Type = 'peq'; ch.eqHighType = 'highshelf';
        ch.eqLowQ = 1.0; ch.eqMid1Q = 1.4; ch.eqMid2Q = 1.4; ch.eqHighQ = 1.0;
        syncLinkedChannel(ch, o => {
          o.eqLowGainDb = 0; o.eqMid1GainDb = 0; o.eqMid2GainDb = 0; o.eqHighGainDb = 0;
          o.eqLowType = 'lowshelf'; o.eqMid1Type = 'peq'; o.eqMid2Type = 'peq'; o.eqHighType = 'highshelf';
          o.eqLowQ = 1.0; o.eqMid1Q = 1.4; o.eqMid2Q = 1.4; o.eqHighQ = 1.0;
        });
        syncChannelAudioParams(ch);
        renderDetailScreen();
      };

      const eqC = document.getElementById('interactiveEqCanvas');
      if (eqC) {
        const freqToNormX = (f) => (Math.log10(Math.max(20, Math.min(20000, f))) - Math.log10(20)) / (Math.log10(20000) - Math.log10(20));
        const normXToFreq = (nx) => Math.pow(10, Math.log10(20) + Math.max(0, Math.min(1, nx)) * (Math.log10(20000) - Math.log10(20)));
        const gainToNormY = (gDb) => (18.0 - Math.max(-18, Math.min(18, gDb))) / 36.0;
        const normYToGain = (ny) => Math.max(-15, Math.min(15, 18.0 - Math.max(0, Math.min(1, ny)) * 36.0));

        let activeNode = state.selectedEqBand;
        bindDrag(eqC,
          (coords) => {
            // Find nearest handle among 0..4
            const pts = [
              { idx: 0, x: freqToNormX(ch.hpfFreq), y: gainToNormY(0) },
              { idx: 1, x: freqToNormX(ch.eqLowFreq), y: gainToNormY((ch.eqLowType === 'lowcut' || ch.eqLowType === 'hicut') ? 0 : ch.eqLowGainDb) },
              { idx: 2, x: freqToNormX(ch.eqMid1Freq), y: gainToNormY((ch.eqMid1Type === 'lowcut' || ch.eqMid1Type === 'hicut') ? 0 : ch.eqMid1GainDb) },
              { idx: 3, x: freqToNormX(ch.eqMid2Freq), y: gainToNormY((ch.eqMid2Type === 'lowcut' || ch.eqMid2Type === 'hicut') ? 0 : ch.eqMid2GainDb) },
              { idx: 4, x: freqToNormX(ch.eqHighFreq), y: gainToNormY((ch.eqHighType === 'lowcut' || ch.eqHighType === 'hicut') ? 0 : ch.eqHighGainDb) }
            ];
            let best = activeNode, bestDist = 999;
            pts.forEach(p => {
              const d = Math.hypot(coords.x - p.x, coords.y - p.y);
              if (d < bestDist) { bestDist = d; best = p.idx; }
            });
            if (bestDist < 0.14) activeNode = best;
            state.selectedEqBand = activeNode;
          },
          (coords) => {
            const f = normXToFreq(coords.x);
            const gDb = normYToGain(coords.y);
            if (activeNode === 0) {
              ch.hpfFreq = Math.min(400, f);
              ch.hpfOn = true;
            } else if (activeNode === 1) {
              ch.eqLowFreq = Math.min(600, f);
              if (ch.eqLowType !== 'lowcut' && ch.eqLowType !== 'hicut') ch.eqLowGainDb = gDb;
              ch.eqOn = true;
            } else if (activeNode === 2) {
              ch.eqMid1Freq = f;
              if (ch.eqMid1Type !== 'lowcut' && ch.eqMid1Type !== 'hicut') ch.eqMid1GainDb = gDb;
              ch.eqOn = true;
            } else if (activeNode === 3) {
              ch.eqMid2Freq = f;
              if (ch.eqMid2Type !== 'lowcut' && ch.eqMid2Type !== 'hicut') ch.eqMid2GainDb = gDb;
              ch.eqOn = true;
            } else if (activeNode === 4) {
              ch.eqHighFreq = Math.max(1000, f);
              if (ch.eqHighType !== 'lowcut' && ch.eqHighType !== 'hicut') ch.eqHighGainDb = gDb;
              ch.eqOn = true;
            }
            syncLinkedChannel(ch, o => {
              if (activeNode === 0) { o.hpfFreq = ch.hpfFreq; o.hpfOn = true; }
              else if (activeNode === 1) { o.eqLowFreq = ch.eqLowFreq; o.eqLowGainDb = ch.eqLowGainDb; o.eqOn = true; }
              else if (activeNode === 2) { o.eqMid1Freq = ch.eqMid1Freq; o.eqMid1GainDb = ch.eqMid1GainDb; o.eqOn = true; }
              else if (activeNode === 3) { o.eqMid2Freq = ch.eqMid2Freq; o.eqMid2GainDb = ch.eqMid2GainDb; o.eqOn = true; }
              else if (activeNode === 4) { o.eqHighFreq = ch.eqHighFreq; o.eqHighGainDb = ch.eqHighGainDb; o.eqOn = true; }
            });
            syncChannelAudioParams(ch);
          },
          () => renderDetailScreen()
        );
      }
    }, 0);
  }

  function drawInteractiveEqCanvas(canvas, ch) {
    if (!canvas) return;
    const dpr = window.devicePixelRatio || 1;
    const w = canvas.clientWidth || 640;
    const h = canvas.clientHeight || 260;
    if (canvas.width !== Math.round(w * dpr) || canvas.height !== Math.round(h * dpr)) {
      canvas.width = Math.round(w * dpr);
      canvas.height = Math.round(h * dpr);
    }
    const g = canvas.getContext('2d');
    g.save();
    g.scale(dpr, dpr);

    // 1. Background
    g.fillStyle = '#0e1014';
    g.fillRect(0, 0, w, h);
    g.strokeStyle = '#242a34';
    g.strokeRect(0.5, 0.5, w - 1, h - 1);

    const leftM = 28;
    const rightM = 8;
    const plotW = w - leftM - rightM;
    const freqToX = (f) => leftM + ((Math.log10(f) - Math.log10(20)) / (Math.log10(20000) - Math.log10(20))) * plotW;
    const gainToY = (db) => 6 + ((18.0 - db) / 36.0) * (h - 22);

    // 2. Real-time RTA Spectrum Bars
    if (state.rtaEnabled && state.rtaAnalyser && state.isPlaying) {
      const freqBins = new Uint8Array(state.rtaAnalyser.frequencyBinCount);
      state.rtaAnalyser.getByteFrequencyData(freqBins);
      const rtaFreqs = [25, 31.5, 40, 50, 63, 80, 100, 125, 160, 200, 250, 315, 400, 500, 630, 800, 1000, 1250, 1600, 2000, 2500, 3150, 4000, 5000, 6300, 8000, 10000, 12500, 16000];
      const sr = state.audioCtx ? state.audioCtx.sampleRate : 48000;
      const barW = Math.max(4, (plotW / rtaFreqs.length) - 2);
      const botY = gainToY(-18);

      rtaFreqs.forEach(rf => {
        const bin = Math.min(freqBins.length - 1, Math.max(1, Math.round((rf / (sr * 0.5)) * freqBins.length)));
        const norm = freqBins[bin] / 255.0;
        const barH = norm * (botY - 14);
        const bx = freqToX(rf) - barW * 0.5;
        g.fillStyle = 'rgba(0, 210, 255, 0.24)';
        g.fillRect(bx, botY - barH, barW, barH);
        g.fillStyle = '#00d2ff';
        g.fillRect(bx, botY - barH - 1.5, barW, 1.5);
      });
    }

    // 3. Frequency Grid
    const gridF = [20, 40, 60, 100, 200, 400, 600, 1000, 2000, 4000, 6000, 10000, 20000];
    const gridL = ['20', '40', '60', '100', '200', '400', '600', '1k', '2k', '4k', '6k', '10k', '20k'];
    g.font = '8px Inter, sans-serif';
    g.textAlign = 'center';
    gridF.forEach((f, i) => {
      const gx = freqToX(f);
      g.strokeStyle = '#1b2028';
      g.beginPath(); g.moveTo(gx, 2); g.lineTo(gx, h - 14); g.stroke();
      g.fillStyle = '#707a8a';
      g.fillText(gridL[i], gx, h - 3);
    });

    // 4. dB Lines
    const dbLines = [15, 10, 5, 0, -5, -10, -15];
    g.textAlign = 'right';
    dbLines.forEach(db => {
      const gy = gainToY(db);
      g.strokeStyle = db === 0 ? '#ffffff' : '#1f2530';
      g.lineWidth = db === 0 ? 1.2 : 1.0;
      g.beginPath(); g.moveTo(leftM, gy); g.lineTo(w - rightM, gy); g.stroke();
      g.fillStyle = db === 0 ? '#ffffff' : '#707a8a';
      g.fillText(db === 0 ? '0 dB' : (db > 0 ? '+' + db : String(db)), leftM - 3, gy + 3);
    });

    // 5. Combined EQ Curve
    const evalGainAt = (f) => {
      let tot = 0;
      if (ch.eqOn) {
        tot += calcBandGainAt(ch.eqLowType || 'lowshelf', f, ch.eqLowFreq, ch.eqLowGainDb, ch.eqLowQ || 1.0);
        tot += calcBandGainAt(ch.eqMid1Type || 'peq', f, ch.eqMid1Freq, ch.eqMid1GainDb, ch.eqMid1Q || 1.4);
        tot += calcBandGainAt(ch.eqMid2Type || 'peq', f, ch.eqMid2Freq, ch.eqMid2GainDb, ch.eqMid2Q || 1.4);
        tot += calcBandGainAt(ch.eqHighType || 'highshelf', f, ch.eqHighFreq, ch.eqHighGainDb, ch.eqHighQ || 1.0);
      }
      if (ch.hpfOn && f < ch.hpfFreq) {
        const octH = Math.log2(ch.hpfFreq / f);
        tot -= 12.0 * octH;
      }
      return Math.max(-18, Math.min(18, tot));
    };

    const zeroY = gainToY(0);
    g.beginPath();
    g.moveTo(leftM, zeroY);
    for (let px = leftM; px <= w - rightM; px += 2) {
      const norm = (px - leftM) / plotW;
      const f = Math.pow(10, Math.log10(20) + norm * (Math.log10(20000) - Math.log10(20)));
      g.lineTo(px, gainToY(evalGainAt(f)));
    }
    g.lineTo(w - rightM, zeroY);
    g.closePath();
    g.fillStyle = ch.eqOn ? 'rgba(0, 210, 255, 0.14)' : 'rgba(120, 130, 145, 0.08)';
    g.fill();

    g.beginPath();
    for (let px = leftM; px <= w - rightM; px += 2) {
      const norm = (px - leftM) / plotW;
      const f = Math.pow(10, Math.log10(20) + norm * (Math.log10(20000) - Math.log10(20)));
      const py = gainToY(evalGainAt(f));
      if (px === leftM) g.moveTo(px, py); else g.lineTo(px, py);
    }
    g.strokeStyle = ch.eqOn ? '#00d2ff' : '#606874';
    g.lineWidth = 2.2;
    g.stroke();

    // 6. Frequency Zone Brackets & Labels at top of plot
    const freqZones = [
      { f1: 20, f2: 80, name: 'SUB (20-80Hz)' },
      { f1: 80, f2: 250, name: 'BASS (80-250Hz)' },
      { f1: 250, f2: 2000, name: 'LO-MID (250-2kHz)' },
      { f1: 2000, f2: 6000, name: 'HI-MID (2-6kHz)' },
      { f1: 6000, f2: 20000, name: 'AIR / TREBLE (6-20kHz)' }
    ];
    freqZones.forEach((z, i) => {
      const x1 = freqToX(z.f1);
      const x2 = freqToX(z.f2);
      g.fillStyle = i % 2 === 0 ? 'rgba(255,255,255,0.02)' : 'rgba(0,210,255,0.035)';
      g.fillRect(x1, 2, x2 - x1, 14);
      g.fillStyle = '#64748b';
      g.font = '7.5px Inter, sans-serif';
      g.textAlign = 'center';
      g.textBaseline = 'top';
      g.fillText(z.name, (x1 + x2) * 0.5, 3.5);
    });

    // 7. 5 Draggable Control Nodes (0=HPF, 1=Low, 2=Mid1, 3=Mid2, 4=High)
    const nodes = [
      { idx: 0, lbl: 'LC', name: 'LO CUT', f: ch.hpfFreq, g: 0, type: 'lowcut', col: '#e53935', range: '20-400Hz: ตัดเสียงลม/หึ่งต่ำ' },
      { idx: 1, lbl: '1', name: '1 LOW', f: ch.eqLowFreq, g: (ch.eqLowType === 'lowcut' || ch.eqLowType === 'hicut') ? 0 : ch.eqLowGainDb, type: ch.eqLowType || 'lowshelf', col: '#00d2ff', range: '20-250Hz: ย่านเบส/กระเดื่อง' },
      { idx: 2, lbl: '2', name: '2 LOMID', f: ch.eqMid1Freq, g: (ch.eqMid1Type === 'lowcut' || ch.eqMid1Type === 'hicut') ? 0 : ch.eqMid1GainDb, type: ch.eqMid1Type || 'peq', col: '#ffc107', range: '250-2kHz: บอดี้/เนื้อเสียงร้อง/สแนร์' },
      { idx: 3, lbl: '3', name: '3 HIMID', f: ch.eqMid2Freq, g: (ch.eqMid2Type === 'lowcut' || ch.eqMid2Type === 'hicut') ? 0 : ch.eqMid2GainDb, type: ch.eqMid2Type || 'peq', col: '#e040fb', range: '1.5-6kHz: ความคมชัด/พุ่งเด่น' },
      { idx: 4, lbl: '4', name: '4 HIGH', f: ch.eqHighFreq, g: (ch.eqHighType === 'lowcut' || ch.eqHighType === 'hicut') ? 0 : ch.eqHighGainDb, type: ch.eqHighType || 'highshelf', col: '#ff9800', range: '4-20kHz: ประกายเสียงแหลม/ฉาบ' }
    ];

    let selNodeData = null;

    nodes.forEach(n => {
      const nx = freqToX(n.f);
      const ny = gainToY(n.g);
      const isSel = state.selectedEqBand === n.idx;
      if (isSel) selNodeData = { ...n, nx, ny };

      // Vertical dashed guide line for active band
      if (isSel) {
        g.strokeStyle = n.col;
        g.setLineDash([2, 3]);
        g.lineWidth = 1;
        g.beginPath();
        g.moveTo(nx, ny);
        g.lineTo(nx, h - 14);
        g.stroke();
        g.setLineDash([]);
      }

      g.beginPath();
      g.arc(nx, ny, isSel ? 9.0 : 6.5, 0, Math.PI * 2);
      g.fillStyle = n.col;
      g.fill();
      g.strokeStyle = isSel ? '#ffffff' : '#0e1014';
      g.lineWidth = isSel ? 2.2 : 1.2;
      g.stroke();

      g.fillStyle = '#000';
      g.font = 'bold 8px Inter, sans-serif';
      g.textAlign = 'center';
      g.textBaseline = 'middle';
      g.fillText(n.lbl, nx, ny);

      // Node frequency & filter type label
      const fStr = n.f >= 1000 ? (n.f / 1000).toFixed(1) + 'k' : Math.round(n.f) + 'Hz';
      const typeStr = (n.type === 'lowcut') ? 'LC' : (n.type === 'hicut' ? 'HC' : (n.type === 'peq' ? 'PEQ' : (n.type === 'lowshelf' ? 'L-SHV' : 'H-SHV')));
      g.font = isSel ? 'bold 8.5px Inter, sans-serif' : '7.5px Inter, sans-serif';
      g.fillStyle = isSel ? '#ffffff' : n.col;
      const lblY = ny > 35 ? ny - 13 : ny + 13;
      g.fillText(`${fStr} [${typeStr}]`, nx, lblY);
    });

    // 8. Active Band HUD Badge at Top Center of Graph
    if (selNodeData) {
      const fStr = selNodeData.f >= 1000 ? (selNodeData.f / 1000).toFixed(2) + ' kHz' : Math.round(selNodeData.f) + ' Hz';
      const gStr = (selNodeData.g >= 0 ? '+' : '') + selNodeData.g.toFixed(1) + ' dB';
      const tLabel = (selNodeData.type === 'lowcut') ? 'LCUT' : (selNodeData.type === 'hicut') ? 'HCUT' : (selNodeData.type === 'peq') ? 'PEQ' : (selNodeData.type === 'lowshelf' ? 'L-SHV' : 'H-SHV');
      const valStr = (selNodeData.type === 'lowcut' || selNodeData.type === 'hicut') ? 'Slope -12dB/oct' : gStr;
      const hudTxt = `${selNodeData.name} [${tLabel}] : ${fStr} (${valStr}) — ${selNodeData.range}`;

      g.font = 'bold 9.5px Inter, sans-serif';
      const txtW = g.measureText(hudTxt).width;
      const hx = Math.max(leftM + 4, Math.min(w - rightM - txtW - 16, selNodeData.nx - (txtW + 16) * 0.5));
      const hy = 18;

      g.fillStyle = 'rgba(18, 22, 30, 0.90)';
      g.fillRect(hx, hy, txtW + 14, 18);
      g.strokeStyle = selNodeData.col;
      g.lineWidth = 1;
      g.strokeRect(hx, hy, txtW + 14, 18);

      g.fillStyle = selNodeData.col;
      g.textAlign = 'left';
      g.textBaseline = 'middle';
      g.fillText(hudTxt, hx + 7, hy + 9);
    }

    g.restore();
  }

  // --- Detail Tab 4: SENDS (16 Bus/FX Send Faders — Matches DetailScreen.cpp setupSendsTab) ---
  function renderDetailSendsTab(card, ch) {
    card.style.display = 'grid';
    card.style.gridTemplateColumns = 'repeat(16, 1fr)';
    card.style.gap = '3px';

    for (let i = 0; i < 16; i++) {
      const isFx = i >= 12;
      const fxSlot = i - 12;
      const col = document.createElement('div');
      col.style.cssText = 'background:#161a21;border:1px solid #283240;border-radius:4px;padding:3px 2px;display:flex;flex-direction:column;align-items:center;gap:2px;';

      const badgeCol = isFx ? '#d81b60' : '#00bcd4';
      const title = isFx ? `Fx${fxSlot + 1}` : `Bus${i + 1}`;
      const sub = isFx ? state.fxSlots[fxSlot].model.shortName : 'POST';

      col.innerHTML = `
        <div style="width:100%;height:26px;background:${badgeCol}33;border:1px solid ${badgeCol};border-radius:3px;text-align:center;display:flex;flex-direction:column;justify-content:center;">
          <div style="font-size:9px;font-weight:800;color:#fff;">${title}</div>
          <div style="font-size:7.5px;color:#cbd5e1;">${sub}</div>
        </div>
        <div style="font-size:8.5px;font-family:var(--font-mono);color:#b0bec5;" id="sendVal-${i}">${ch.sendsDb[i] <= -89 ? '-oo' : ch.sendsDb[i].toFixed(1)}</div>
        <canvas class="ch-fader-canvas" id="sendFader-${i}" style="width:100%;flex:1;"></canvas>
        <button class="ch-sm-btn mute ${ch.sendsMute[i] ? 'on' : ''}" id="sendMute-${i}" style="width:88%;height:20px;">M</button>
      `;
      card.appendChild(col);

      setTimeout(() => {
        const fC = document.getElementById(`sendFader-${i}`);
        const vEl = document.getElementById(`sendVal-${i}`);
        const mBtn = document.getElementById(`sendMute-${i}`);
        drawFaderCanvas(fC, ch.sendsDb[i], isFx);

        const onMove = (c) => {
          ch.sendsDb[i] = faderPosToDb(1.0 - c.y);
          syncLinkedChannel(ch, o => { o.sendsDb[i] = ch.sendsDb[i]; });
          if (vEl) vEl.textContent = ch.sendsDb[i] <= -89 ? '-oo' : ch.sendsDb[i].toFixed(1);
          drawFaderCanvas(fC, ch.sendsDb[i], isFx);
          syncChannelAudioParams(ch);
        };
        bindDrag(fC, onMove, onMove, null, () => {
          ch.sendsDb[i] = 0.0;
          syncLinkedChannel(ch, o => { o.sendsDb[i] = 0.0; });
          if (vEl) vEl.textContent = '0.0';
          drawFaderCanvas(fC, 0.0, isFx);
          syncChannelAudioParams(ch);
        });

        if (mBtn) {
          mBtn.onclick = () => {
            ch.sendsMute[i] = !ch.sendsMute[i];
            syncLinkedChannel(ch, o => { o.sendsMute[i] = ch.sendsMute[i]; });
            syncChannelAudioParams(ch);
            renderDetailScreen();
          };
        }
      }, 0);
    }
  }

  // --- Detail Tab 5: NAMING ---
  function renderDetailNamingTab(card, ch) {
    card.style.flexDirection = 'column';
    card.style.padding = '16px';
    card.style.gap = '14px';
    card.innerHTML = `
      <div class="detail-box-title" style="font-size:11px;">CHANNEL NAME & SCRIBBLE STRIP COLOR</div>
      <div style="display:flex;align-items:center;gap:10px;">
        <input type="text" id="chNameInput" value="${ch.name}" style="width:240px;height:32px;background:#12151a;border:1px solid #3d4756;border-radius:4px;color:#fff;padding:0 10px;font-size:13px;font-weight:700;" />
        <button class="cbtn active-cyan" id="btnApplyName" style="width:90px;height:32px;">APPLY</button>
      </div>
      <div style="display:flex;gap:8px;" id="colorSwatchRow"></div>
    `;

    setTimeout(() => {
      const inp = document.getElementById('chNameInput');
      const btn = document.getElementById('btnApplyName');
      if (btn && inp) {
        btn.onclick = () => {
          ch.name = inp.value.trim() || (`CH ${String(ch.index + 1).padStart(2, '0')}`);
          renderDetailScreen();
        };
      }
      const row = document.getElementById('colorSwatchRow');
      kChannelPalette.forEach(col => {
        const b = document.createElement('button');
        b.style.cssText = `width:38px;height:38px;border-radius:4px;background:${col};border:${ch.color === col ? '3px solid #fff' : '1px solid #000'};cursor:pointer;`;
        b.onclick = () => { ch.color = col; renderDetailScreen(); };
        row.appendChild(b);
      });
    }, 0);
  }

  // --- Detail Tab 6: PRESET ---
  function renderDetailPresetTab(card, ch) {
    card.style.flexDirection = 'column';
    card.style.padding = '12px';
    card.innerHTML = `<div class="detail-box-title" style="font-size:11px;margin-bottom:8px;">CHANNEL STRIP DSP PRESETS</div>`;
    const grid = document.createElement('div');
    grid.style.cssText = 'display:grid;grid-template-columns:repeat(3,1fr);gap:8px;flex:1;';

    const presets = [
      { name: 'Flat / Reset Channel', fn: () => { ch.hpfOn = false; ch.eqLowGainDb = 0; ch.eqMid1GainDb = 0; ch.eqMid2GainDb = 0; ch.eqHighGainDb = 0; ch.compOn = false; ch.gateOn = false; } },
      { name: 'Punchy Kick Drum', fn: () => { ch.hpfOn = true; ch.hpfFreq = 35; ch.eqLowFreq = 65; ch.eqLowGainDb = 4.5; ch.eqMid1Freq = 320; ch.eqMid1GainDb = -5.0; ch.eqHighFreq = 4500; ch.eqHighGainDb = 3.5; ch.compOn = true; ch.compThreshDb = -16; ch.compRatio = 4.5; } },
      { name: 'Crisp Studio Snare', fn: () => { ch.hpfOn = true; ch.hpfFreq = 85; ch.eqLowFreq = 180; ch.eqLowGainDb = 2.5; ch.eqMid2Freq = 3500; ch.eqMid2GainDb = 4.0; ch.compOn = true; ch.compThreshDb = -18; ch.compRatio = 4.0; } },
      { name: 'Tight Electric Bass', fn: () => { ch.hpfOn = true; ch.hpfFreq = 40; ch.eqLowFreq = 80; ch.eqLowGainDb = 3.5; ch.eqMid1Freq = 750; ch.eqMid1GainDb = 2.0; ch.compOn = true; ch.compThreshDb = -20; ch.compRatio = 6.0; ch.compMakeupDb = 3.0; } },
      { name: 'Clear Lead Vocal', fn: () => { ch.hpfOn = true; ch.hpfFreq = 110; ch.eqLowGainDb = -2.0; ch.eqMid1Freq = 350; ch.eqMid1GainDb = -2.5; ch.eqMid2Freq = 3200; ch.eqMid2GainDb = 3.0; ch.eqHighFreq = 10000; ch.eqHighGainDb = 4.0; ch.compOn = true; ch.compThreshDb = -22; ch.compRatio = 3.5; } },
      { name: 'Warm Keys / Acoustic', fn: () => { ch.hpfOn = true; ch.hpfFreq = 90; ch.eqLowGainDb = 1.5; ch.eqMid1Freq = 500; ch.eqMid1GainDb = -2.0; ch.eqHighFreq = 8000; ch.eqHighGainDb = 3.0; } }
    ];

    presets.forEach(p => {
      const b = document.createElement('button');
      b.className = 'cbtn';
      b.style.fontSize = '11.5px';
      b.textContent = p.name;
      b.onclick = () => {
        p.fn();
        syncChannelAudioParams(ch);
        showToast(`ใช้พรีเซ็ต "${p.name}" กับ ${ch.name} แล้ว`);
      };
      grid.appendChild(b);
    });
    card.appendChild(grid);
  }

  // ============================================================================
  // SCREEN 3: EFFECTS SCREEN (Exact X32 Virtual FX Rack & Lexicon 480L LARC)
  // ============================================================================
  function renderEffectsScreen() {
    const screen = document.getElementById('screen-effects');
    if (!screen) return;
    screen.innerHTML = '';

    const slot = state.fxSlots[state.selectedFxSlot];
    const model = slot.model;

    // 1. Top Header Bar (Matches EffectsScreen.cpp paint)
    const hdr = document.createElement('div');
    hdr.className = 'fx-top-hdr';
    hdr.innerHTML = `
      <div style="display:flex;align-items:center;gap:10px;">
        <button class="cbtn" style="width:68px;height:22px;background:#27272a;color:#fff;" onclick="MPS.showScreen(0)">&lt; Mixer</button>
        <div style="width:10px;height:10px;border-radius:50%;background:#ef4444;box-shadow:0 0 8px #ef4444;"></div>
        <span style="font-size:12.5px;font-weight:800;color:#fff;">BEHRINGER</span>
        <span style="font-size:11px;color:#71717a;">|  X32 VIRTUAL FX RACK</span>
      </div>
      <div style="display:flex;align-items:center;gap:8px;padding-right:34px;">
        <div style="background:#27272a;border:1px solid #3f3f46;border-radius:4px;padding:2px 10px;font-family:var(--font-mono);font-size:9.5px;font-weight:700;color:#38bdf8;">
          FX1-4: 4 ACTIVE MODELS (LITE)
        </div>
        <button class="cbtn" id="btnFxLib" style="height:22px;padding:0 12px;background:#0284c7;border-color:#38bdf8;color:#fff;font-size:10px;">
          ⚡ FX1-4 LIBRARY
        </button>
      </div>
    `;
    screen.appendChild(hdr);

    // 2. Main Rack Chassis
    const chassis = document.createElement('div');
    chassis.className = 'fx-chassis';

    // FX1..FX8 Slot Tabs (FX 1..4 active, FX 5..8 Lite locked)
    const tabsRow = document.createElement('div');
    tabsRow.className = 'fx-slot-tabs';
    for (let i = 0; i < 8; i++) {
      const tb = document.createElement('button');
      const isLocked = i >= 4;
      tb.className = `fx-slot-tab ${state.selectedFxSlot === i ? 'active' : ''} ${isLocked ? 'locked' : ''}`;
      tb.textContent = 'FX' + (i + 1);
      tb.onclick = () => {
        if (isLocked) {
          showToast('รุ่น Practice Lite เปิดใช้งานเอฟเฟกต์ได้ 4 ช่อง (FX1 - FX4)');
          return;
        }
        state.selectedFxSlot = i;
        renderEffectsScreen();
      };
      tabsRow.appendChild(tb);
    }
    chassis.appendChild(tabsRow);

    // Rack Routing Strip (Insert Bus, 11-seg LED meters, [ L oo R ], MUTE, Title, EFFECTS/PRESETS)
    const rStrip = document.createElement('div');
    rStrip.className = 'fx-routing-strip';
    rStrip.innerHTML = `
      <div style="display:flex;align-items:center;gap:8px;background:#303033;border:1px solid #8a8a93;border-radius:3px;padding:3px 6px;height:100%;">
        <div style="display:flex;flex-direction:column;gap:2px;">
          <button class="cbtn" style="width:68px;height:17px;font-size:9px;background:#222;">Bus ${13 + state.selectedFxSlot}</button>
          <button class="cbtn" style="width:68px;height:17px;font-size:9px;background:#222;">Bus ${13 + state.selectedFxSlot}</button>
        </div>
        <canvas id="fxInOutMetersCanvas" style="width:310px;height:34px;"></canvas>
        <div style="display:flex;flex-direction:column;gap:2px;">
          <button class="cbtn ${slot.bypass ? 'active-red' : ''}" id="btnFxMute1" style="width:64px;height:17px;font-size:9px;">MUTE</button>
          <button class="cbtn ${slot.bypass ? 'active-red' : ''}" id="btnFxMute2" style="width:64px;height:17px;font-size:9px;">MUTE</button>
        </div>
      </div>
      <div style="display:flex;align-items:center;gap:10px;">
        <span style="font-family:var(--font-mono);font-size:11.5px;font-weight:800;color:#fbbf24;">
          FX${state.selectedFxSlot + 1}: ${model.name.toUpperCase()}
        </span>
        <div style="display:flex;flex-direction:column;gap:2px;">
          <button class="cbtn" id="btnFxModelCycle" style="width:84px;height:18px;font-size:9.5px;">EFFECTS</button>
          <button class="cbtn" id="btnFxPresetCycle" style="width:84px;height:18px;font-size:9.5px;">PRESETS</button>
        </div>
      </div>
    `;
    chassis.appendChild(rStrip);

    // Center Hardware Faceplate
    const faceWrap = document.createElement('div');
    faceWrap.className = 'fx-unit-faceplate';

    if (model.family === 'larc') {
      // Lexicon 480L LARC Beige Faceplate (Matches effects_larc_page0_1024x642.png)
      const larc = document.createElement('div');
      larc.className = 'larc-faceplate';

      const bezel = document.createElement('div');
      bezel.className = 'larc-vfd-bezel';
      model.params.forEach((p, pIdx) => {
        const col = document.createElement('div');
        col.className = 'larc-vfd-col';
        col.innerHTML = `
          <div class="larc-vfd-name">${p.name}</div>
          <div class="larc-vfd-val" id="larcVal-${pIdx}">${p.val} ${p.unit}</div>
        `;
        bezel.appendChild(col);
      });
      larc.appendChild(bezel);

      const slidersRow = document.createElement('div');
      slidersRow.className = 'larc-sliders-row';
      model.params.forEach((p, pIdx) => {
        const sCanvas = document.createElement('canvas');
        sCanvas.style.cssText = 'width:100%;height:100%;display:block;cursor:ns-resize;';
        slidersRow.appendChild(sCanvas);

        const drawLarcSlider = () => {
          const dpr = window.devicePixelRatio || 1;
          const w = sCanvas.clientWidth || 120;
          const h = sCanvas.clientHeight || 160;
          sCanvas.width = w * dpr; sCanvas.height = h * dpr;
          const g = sCanvas.getContext('2d');
          g.save();
          g.scale(dpr, dpr);

          const slotX = 22;
          const topY = 10;
          const botY = h - 10;
          const spanH = botY - topY;

          // 11 Horizontal Tick Lines on the right of the fader
          g.fillStyle = '#2b2822';
          for (let t = 0; t <= 10; t++) {
            const ty = topY + (t / 10) * spanH;
            g.fillRect(slotX + 14, ty, w - slotX - 24, 1.5);
          }

          // Vertical dark slot
          g.fillStyle = '#12110e';
          g.fillRect(slotX - 2, topY, 4, spanH);

          // Cream LARC Fader Cap
          const norm = (p.val - p.min) / Math.max(1e-5, p.max - p.min);
          const cy = botY - norm * spanH;
          const capW = 36, capH = 18;
          const grad = g.createLinearGradient(0, cy - capH * 0.5, 0, cy + capH * 0.5);
          grad.addColorStop(0, '#e5dec9');
          grad.addColorStop(1, '#b5aa90');
          g.fillStyle = grad;
          g.beginPath();
          g.roundRect(slotX - capW * 0.5, cy - capH * 0.5, capW, capH, 3);
          g.fill();
          g.strokeStyle = '#3d372c';
          g.lineWidth = 1.2;
          g.stroke();

          g.fillStyle = '#2b261f';
          g.fillRect(slotX - capW * 0.5 + 4, cy - 0.8, capW - 8, 1.6);
          g.restore();
        };

        const onDragLarc = (coords) => {
          const norm = Math.max(0, Math.min(1, 1.0 - coords.y));
          const raw = p.min + norm * (p.max - p.min);
          p.val = (p.max - p.min <= 10) ? Number(raw.toFixed(2)) : Math.round(raw);
          const vEl = document.getElementById(`larcVal-${pIdx}`);
          if (vEl) vEl.textContent = `${p.val} ${p.unit}`;
          drawLarcSlider();
          applyFxSlotAudioParams(slot);
        };
        bindDrag(sCanvas, onDragLarc, onDragLarc);
        setTimeout(drawLarcSlider, 0);
      });
      larc.appendChild(slidersRow);

      const pBtnsRow = document.createElement('div');
      pBtnsRow.className = 'larc-preset-btns-row';
      ['HALL', 'AMBIENCE', 'PLATE', 'ROOM', 'CHAMBER', 'CONCERT'].forEach((bName, bIdx) => {
        const b = document.createElement('button');
        b.className = `larc-pbtn ${state.larcSubPreset === bIdx ? 'active' : ''}`;
        b.textContent = bName;
        b.onclick = () => {
          state.larcSubPreset = bIdx;
          slot.model.name = bName + ' REVERB';
          slot.model.params[1].val = [3.2, 0.8, 1.8, 1.2, 2.2, 4.5][bIdx];
          applyFxSlotAudioParams(slot);
          renderEffectsScreen();
        };
        pBtnsRow.appendChild(b);
      });
      larc.appendChild(pBtnsRow);
      faceWrap.appendChild(larc);
    } else {
      // Dark Teal Hardware Faceplate with Rotary Dials (Matches effects_slot6_1024x642.png)
      const tealPlate = document.createElement('div');
      tealPlate.style.cssText = 'width:100%;height:100%;background:linear-gradient(180deg,#20434d 0%,#162e36 100%);border:2px solid #2d5561;padding:10px 18px;display:flex;flex-direction:column;';
      tealPlate.innerHTML = `<div style="font-size:12px;font-weight:800;color:#e0f2fe;letter-spacing:0.6px;margin-bottom:6px;">${model.name.toUpperCase()}  •  STUDIO PROCESSOR</div>`;
      const knobsGrid = document.createElement('div');
      knobsGrid.style.cssText = 'flex:1;display:grid;grid-template-columns:repeat(6,1fr);align-items:center;justify-items:center;';
      model.params.forEach(p => {
        knobsGrid.appendChild(createConsoleDialElement({
          title: p.name, min: p.min, max: p.max, def: p.val, value: p.val, size: 86,
          format: v => (p.max - p.min <= 10 ? v.toFixed(1) : Math.round(v)) + ' ' + p.unit,
          onChange: v => {
            p.val = v;
            applyFxSlotAudioParams(slot);
          }
        }));
      });
      tealPlate.appendChild(knobsGrid);
      faceWrap.appendChild(tealPlate);
    }
    chassis.appendChild(faceWrap);

    // Bottom Rack Bar
    const botBar = document.createElement('div');
    botBar.className = 'fx-bottom-bar';
    botBar.innerHTML = `
      <div style="display:flex;gap:6px;">
        <button class="cbtn" style="width:64px;height:22px;font-size:9.5px;">Copy</button>
        <button class="cbtn" style="width:64px;height:22px;font-size:9.5px;">Paste</button>
      </div>
      <span style="font-family:var(--font-mono);font-size:10px;color:#a1a1aa;">
        BEHRINGER X32 VIRTUAL FX RACK  |  ENGINE 1 ACTIVE
      </span>
      <div style="display:flex;align-items:center;gap:6px;">
        <span style="font-size:10px;font-weight:700;color:#a1a1aa;">LAYER / ENGINE</span>
        <button class="cbtn active-cyan" style="width:30px;height:24px;">1</button>
        <button class="cbtn" style="width:30px;height:24px;">2</button>
      </div>
    `;
    chassis.appendChild(botBar);
    screen.appendChild(chassis);

    setTimeout(() => {
      const cycleModel = () => {
        const curIdx = kFxCatalog.findIndex(m => m.id === slot.model.id);
        const nextIdx = (curIdx + 1) % kFxCatalog.length;
        slot.model = JSON.parse(JSON.stringify(kFxCatalog[nextIdx]));
        applyFxSlotAudioParams(slot);
        renderEffectsScreen();
      };
      document.getElementById('btnFxLib').onclick = cycleModel;
      document.getElementById('btnFxModelCycle').onclick = cycleModel;
      document.getElementById('btnFxPresetCycle').onclick = () => {
        showToast(`โหลดค่าเริ่มต้นของ ${slot.model.name} แล้ว`);
      };
      const toggleMute = () => {
        slot.bypass = !slot.bypass;
        applyFxSlotAudioParams(slot);
        renderEffectsScreen();
      };
      document.getElementById('btnFxMute1').onclick = toggleMute;
      document.getElementById('btnFxMute2').onclick = toggleMute;
    }, 0);
  }

  function applyFxSlotAudioParams(slot) {
    if (!state.audioCtx || !slot.nodes) return;
    const t = state.audioCtx.currentTime;
    const n = slot.nodes;
    n.outGain.gain.setTargetAtTime(slot.bypass ? 0.0 : 0.45, t, 0.02);
    if (n.preDelay && slot.model.params[0]) {
      n.preDelay.delayTime.setTargetAtTime(slot.model.params[0].val / 1000.0, t, 0.02);
    }
    if (n.damp && slot.model.params[3]) {
      n.damp.frequency.setTargetAtTime(slot.model.params[3].val * 1000.0, t, 0.02);
    }
    if (n.dly && slot.model.params[0]) {
      n.dly.delayTime.setTargetAtTime(slot.model.params[0].val / 1000.0, t, 0.02);
    }
    if (n.fb && slot.model.params[1]) {
      n.fb.gain.setTargetAtTime(slot.model.params[1].val / 100.0, t, 0.02);
    }
    if (n.lfo && slot.model.params[0]) {
      n.lfo.frequency.setTargetAtTime(slot.model.params[0].val, t, 0.02);
    }
  }

  // ============================================================================
  // SCREEN 2: TRACKS SCREEN (Max 16 Tracks — Matches TracksScreen.cpp)
  // ============================================================================
  function renderTracksScreen() {
    const screen = document.getElementById('screen-tracks');
    if (!screen) return;
    screen.innerHTML = '';

    const cfg = state.importWindowConfig || { startMin: 0, startSec: 0, durMin: 5, durSec: 0, fullTrack: false };
    const cfgStr = cfg.fullTrack ? 'FULL SONG' : `${String(cfg.startMin).padStart(2, '0')}:${String(cfg.startSec).padStart(2, '0')} (+${cfg.durMin}m)`;

    const topBar = document.createElement('div');
    topBar.className = 'tracks-top-bar';
    topBar.innerHTML = `
      <button class="cbtn" style="width:110px;height:26px;background:#202630;color:#00d2ff;" onclick="MPS.showScreen(0)">&lt; BACK TO MIXER</button>
      <button class="cbtn active-cyan" style="padding:0 12px;height:26px;" onclick="document.getElementById('hiddenAudioInput').click()">
        IMPORT AUDIO FILES (MAX 16)
      </button>
      <button class="cbtn" style="padding:0 12px;height:26px;background:#003847;border:1px solid #00d2ff;color:#00e5ff;font-weight:700;" onclick="MPS.openSectionSlicerModal(false)" title="เลือกท่อนเพลง 0-25%, 25-50%, 50-75%, 75-100% หรือทั้งเพลง">
        ✂️ ท่อนเพลง: ${state.activeSection.label}
      </button>
      <button class="cbtn" style="padding:0 12px;height:26px;background:#005c6e;color:#fff;border-color:#00bcd4;" onclick="MPS.saveProject()">
        💾 SAVE PROJECT (.mpsproj)
      </button>
      <button class="cbtn" style="padding:0 12px;height:26px;background:#202630;color:#00e5ff;border-color:#384454;" onclick="MPS.loadProject()">
        📂 LOAD PROJECT
      </button>
      <button class="cbtn" style="padding:0 12px;height:26px;background:#1565c0;border-color:#42a5f5;color:#fff;" onclick="MPS.loadDemoStems()">
        🎵 LOAD DEMO (8 STEMS)
      </button>
      <button class="cbtn" style="padding:0 12px;height:26px;background:#4a1a1c;color:#fff;" id="btnClearAllTracks">
        CLEAR ALL
      </button>
      <span style="margin-left:auto;font-size:10.5px;color:#8b949e;">Practice Lite: Supports up to 16 synchronized audio tracks</span>
    `;
    screen.appendChild(topBar);

    const tableWrap = document.createElement('div');
    tableWrap.className = 'tracks-table-wrap';
    tableWrap.innerHTML = `
      <div class="tracks-table-hdr">
        <div>Move</div>
        <div>Slot</div>
        <div>File Name</div>
        <div>Channels</div>
        <div>Duration</div>
        <div>Sample Rate</div>
        <div>Assigned To</div>
        <div>Clear</div>
      </div>
      <div class="tracks-table-body allow-scroll" id="tracksTableBody"></div>
    `;
    screen.appendChild(tableWrap);

    const tbody = tableWrap.querySelector('#tracksTableBody');
    state.channels.forEach((ch, idx) => {
      const row = document.createElement('div');
      row.className = `track-row ${ch.buffer ? 'loaded' : ''}`;
      row.innerHTML = `
        <div style="display:flex;gap:3px;align-items:center;">
          <button class="cbtn" style="width:18px;height:18px;font-size:8px;" onclick="MPS.moveTrack(${idx}, -1)">▲</button>
          <span style="color:#00d2ff;">⠿</span>
          <button class="cbtn" style="width:18px;height:18px;font-size:8px;" onclick="MPS.moveTrack(${idx}, 1)">▼</button>
        </div>
        <div style="color:#00d2ff;font-weight:700;">#${String(idx + 1).padStart(2, '0')}</div>
        <div style="font-weight:600;white-space:nowrap;overflow:hidden;text-overflow:ellipsis;">
          ${ch.buffer ? (ch.fileName || ch.name) : '<span style="color:#606874;">-- Empty Slot --</span>'}
        </div>
        <div>${ch.buffer ? (ch.buffer.numberOfChannels === 2 ? 'Stereo (2)' : 'Mono (1)') : '-'}</div>
        <div style="font-family:var(--font-mono);">${ch.buffer ? formatTimeSec(ch.buffer.duration) : '--:--'}</div>
        <div>${ch.buffer ? (ch.buffer.sampleRate / 1000).toFixed(1) + ' kHz' : '-'}</div>
        <div style="color:#ffb74d;font-weight:700;">CH ${String(idx + 1).padStart(2, '0')} (${ch.name})</div>
        <div>
          <button class="cbtn" style="width:32px;height:18px;background:#3a1f22;color:#ff5252;" onclick="MPS.clearTrackSlot(${idx})">✕</button>
        </div>
      `;
      tbody.appendChild(row);
    });

    setTimeout(() => {
      const clrBtn = document.getElementById('btnClearAllTracks');
      if (clrBtn) {
        clrBtn.onclick = async () => {
          MPS.stop();
          state.rawFiles = [];
          await clearAllStoredRawStems();
          state.channels.forEach((c, i) => {
            c.buffer = null;
            c.fileName = '';
            c.name = 'CH ' + String(i + 1).padStart(2, '0');
          });
          state.totalDurationSec = 0;
          renderTracksScreen();
          renderTopNav();
          showToast('ล้างแทร็กเสียงทั้งหมดเรียบร้อย');
        };
      }
    }, 0);
  }

  // ============================================================================
  // SCREEN 4: SCENES SCREEN (Matches ScenesScreen.cpp)
  // ============================================================================
  function renderScenesScreen() {
    const screen = document.getElementById('screen-scenes');
    if (!screen) return;
    screen.innerHTML = '';

    const wrap = document.createElement('div');
    wrap.style.cssText = 'flex:1;display:grid;grid-template-columns:300px 1fr;gap:10px;min-height:0;';

    // Left Box: Project File Storage
    const leftBox = document.createElement('div');
    leftBox.className = 'detail-sub-box';
    leftBox.style.padding = '12px';
    leftBox.style.gap = '8px';
    leftBox.innerHTML = `
      <div style="display:flex;align-items:center;justify-content:space-between;">
        <span style="font-size:13px;font-weight:800;color:#fff;">Project File Storage</span>
        <button class="cbtn" style="width:74px;height:24px;color:#00d2ff;" onclick="MPS.showScreen(0)">&lt; BACK</button>
      </div>
      <p style="font-size:11px;color:#8b949e;line-height:1.4;">บันทึกและโหลดโปรเจกต์มิกซ์พร้อมไฟล์เสียงทั้งหมด (.mpsproj) ลงในโฟลเดอร์ดาวน์โหลด</p>
      <button class="cbtn active-cyan" style="height:32px;font-weight:bold;background:#00bcd4;color:#000;" onclick="MPS.saveProject()">💾 SAVE PROJECT BUNDLE (.mpsproj)</button>
      <button class="cbtn" style="height:32px;font-weight:bold;border-color:#00bcd4;color:#00e5ff;" onclick="MPS.loadProject()">📂 LOAD PROJECT FILE (.mpsproj / .json)</button>
      <button class="cbtn" style="height:24px;background:#2a3340;color:#94a3b8;font-size:10px;" onclick="MPS.exportSceneJson()">EXPORT SETTINGS ONLY (.json)</button>
      <button class="cbtn" style="height:24px;background:#00838f;color:#fff;font-size:10px;" onclick="MPS.quickSaveSession()">QUICK SAVE TO BROWSER</button>
    `;
    wrap.appendChild(leftBox);

    // Right Box: Console Scene Snapshots
    const rightBox = document.createElement('div');
    rightBox.className = 'detail-sub-box';
    rightBox.style.padding = '12px';
    rightBox.style.gap = '8px';
    rightBox.innerHTML = `
      <div style="font-size:13px;font-weight:800;color:#fff;">Console Scene Snapshots</div>
      <div style="display:flex;gap:6px;height:28px;">
        <input type="text" id="newSceneNameInput" value="Scene ${state.scenes.length + 1} - Custom Mix" style="flex:1;background:#121417;border:1px solid #323842;border-radius:4px;color:#fff;padding:0 8px;font-size:11.5px;" />
        <button class="cbtn active-cyan" style="padding:0 12px;" id="btnCaptureScene">CAPTURE SCENE</button>
        <button class="cbtn active-yellow" style="padding:0 12px;" id="btnRecallScene">RECALL SCENE</button>
      </div>
      <div style="flex:1;background:#121417;border:1px solid #282f3a;border-radius:4px;overflow-y:auto;" class="allow-scroll" id="scenesListBox"></div>
    `;
    wrap.appendChild(rightBox);
    screen.appendChild(wrap);

    const listEl = rightBox.querySelector('#scenesListBox');
    state.scenes.forEach((sc, idx) => {
      const item = document.createElement('div');
      const isSel = state.selectedSceneIdx === idx;
      item.style.cssText = `padding:7px 10px;font-size:12px;font-weight:600;cursor:pointer;background:${isSel ? 'rgba(0,210,255,0.25)' : (idx % 2 === 1 ? 'rgba(35,39,46,0.4)' : 'transparent')};color:#e6e8eb;`;
      item.textContent = sc.name;
      item.onclick = () => { state.selectedSceneIdx = idx; renderScenesScreen(); };
      listEl.appendChild(item);
    });

    setTimeout(() => {
      document.getElementById('btnCaptureScene').onclick = () => {
        const inp = document.getElementById('newSceneNameInput');
        const name = (inp && inp.value.trim()) || `Scene ${state.scenes.length + 1}`;
        state.scenes.push({ name, snapshot: captureSnapshotData() });
        state.selectedSceneIdx = state.scenes.length - 1;
        renderScenesScreen();
        showToast(`บันทึกซีน "${name}" เรียบร้อย`);
      };
      document.getElementById('btnRecallScene').onclick = () => {
        const sc = state.scenes[state.selectedSceneIdx];
        if (!sc) return;
        applySceneSnapshot(sc.snapshot);
        showToast(`เรียกคืนซีน "${sc.name}" เรียบร้อย`);
      };
    }, 0);
  }

  function captureSnapshotData() {
    return {
      mainFaderDb: state.main.faderDb,
      channelPairLinked: [...state.channelPairLinked],
      activeSection: state.activeSection,
      loopStartNorm: state.loopStartNorm,
      loopEndNorm: state.loopEndNorm,
      scenes: state.scenes.map(s => ({ name: s.name, snapshot: s.snapshot })),
      channels: state.channels.map(c => ({
        name: c.name,
        fileName: c.fileName,
        color: c.color,
        trimDb: c.trimDb, faderDb: c.faderDb, pan: c.pan, mute: c.mute, solo: c.solo,
        linked: c.linked,
        delayOn: c.delayOn, delayMs: c.delayMs,
        hpfOn: c.hpfOn, hpfFreq: c.hpfFreq, eqOn: c.eqOn,
        eqLowType: c.eqLowType || 'lowshelf', eqLowFreq: c.eqLowFreq, eqLowGainDb: c.eqLowGainDb, eqLowQ: c.eqLowQ !== undefined ? c.eqLowQ : 1.0,
        eqMid1Type: c.eqMid1Type || 'peq', eqMid1Freq: c.eqMid1Freq, eqMid1GainDb: c.eqMid1GainDb, eqMid1Q: c.eqMid1Q !== undefined ? c.eqMid1Q : 1.4,
        eqMid2Type: c.eqMid2Type || 'peq', eqMid2Freq: c.eqMid2Freq, eqMid2GainDb: c.eqMid2GainDb, eqMid2Q: c.eqMid2Q !== undefined ? c.eqMid2Q : 1.4,
        eqHighType: c.eqHighType || 'highshelf', eqHighFreq: c.eqHighFreq, eqHighGainDb: c.eqHighGainDb, eqHighQ: c.eqHighQ !== undefined ? c.eqHighQ : 1.0,
        gateOn: c.gateOn, gateThreshDb: c.gateThreshDb, gateRangeDb: c.gateRangeDb,
        gateAttackMs: c.gateAttackMs, gateHoldMs: c.gateHoldMs, gateReleaseMs: c.gateReleaseMs,
        compOn: c.compOn, compThreshDb: c.compThreshDb, compRatio: c.compRatio,
        compMakeupDb: c.compMakeupDb, compKneeDb: c.compKneeDb,
        compAttackMs: c.compAttackMs, compHoldMs: c.compHoldMs, compReleaseMs: c.compReleaseMs, compMixPct: c.compMixPct,
        sendsDb: [...c.sendsDb], sendsMute: [...c.sendsMute]
      }))
    };
  }

  function applySceneSnapshot(snap) {
    if (!snap) {
      state.channelPairLinked.fill(false);
      state.channels.forEach(c => {
        c.faderDb = 0; c.pan = 0; c.mute = false; c.solo = false; c.linked = false;
        c.eqLowGainDb = 0; c.eqMid1GainDb = 0; c.eqMid2GainDb = 0; c.eqHighGainDb = 0;
        c.compOn = false; c.gateOn = false;
      });
    } else if (snap === 'preset_folk') {
      state.channelPairLinked.fill(false);
      const names = ['Kick', 'Snare', 'HiHat', 'Ponglang', 'Pin Solo', 'Kaen', 'Bass', 'Vocal'];
      names.forEach((n, i) => {
        state.channels[i].name = n;
        state.channels[i].linked = false;
        state.channels[i].faderDb = [1.5, 0.5, -3.0, 2.0, 1.5, 1.0, 0.0, 2.5][i];
        state.channels[i].pan = [0, 0, -0.3, -0.45, 0.45, -0.25, 0, 0][i];
      });
    } else if (snap === 'preset_rock') {
      state.channelPairLinked.fill(false);
      state.channels[0].faderDb = 2.5; state.channels[0].eqLowGainDb = 4.0;
      state.channels[1].faderDb = 1.5; state.channels[1].compOn = true;
      state.channels[4].faderDb = 1.0; state.channels[4].eqLowGainDb = 3.0;
    } else if (snap === 'preset_vocal') {
      state.channelPairLinked.fill(false);
      state.channels.forEach(c => { c.sendsDb[12] = -90.0; c.eqHighGainDb = 2.5; });
    } else if (snap.channels) {
      state.main.faderDb = snap.mainFaderDb || 0;
      if (snap.channelPairLinked) {
        state.channelPairLinked = [...snap.channelPairLinked];
      }
      if (snap.activeSection) {
        state.activeSection = snap.activeSection;
      }
      if (snap.loopStartNorm !== undefined) state.loopStartNorm = snap.loopStartNorm;
      if (snap.loopEndNorm !== undefined) state.loopEndNorm = snap.loopEndNorm;
      if (snap.scenes && Array.isArray(snap.scenes)) {
        state.scenes = snap.scenes;
      }
      snap.channels.forEach((sc, i) => {
        if (state.channels[i]) {
          Object.assign(state.channels[i], sc);
          state.channels[i].linked = isChannelLinked(i);
          if (!state.channels[i].eqLowType) state.channels[i].eqLowType = 'lowshelf';
          if (!state.channels[i].eqMid1Type) state.channels[i].eqMid1Type = 'peq';
          if (!state.channels[i].eqMid2Type) state.channels[i].eqMid2Type = 'peq';
          if (!state.channels[i].eqHighType) state.channels[i].eqHighType = 'highshelf';
          if (state.channels[i].eqLowQ === undefined) state.channels[i].eqLowQ = 1.0;
          if (state.channels[i].eqHighQ === undefined) state.channels[i].eqHighQ = 1.0;
        }
      });
    }
    syncAllAudioParams();
  }

  // ============================================================================
  // MULTITRACK PROJECT PACKER & UNPACKER (.mpsproj / ZIP Archive)
  // Bundles JSON Mixer Parameters + Audio Stems into a single downloadable file
  // ============================================================================
  const kCrcTable = (() => {
    const t = new Uint32Array(256);
    for (let i = 0; i < 256; i++) {
      let c = i;
      for (let k = 0; k < 8; k++) c = (c & 1) ? (0xEDB88320 ^ (c >>> 1)) : (c >>> 1);
      t[i] = c;
    }
    return t;
  })();

  function calcCrc32(uint8Array) {
    let crc = 0 ^ (-1);
    for (let i = 0; i < uint8Array.length; i++) crc = (crc >>> 8) ^ kCrcTable[(crc ^ uint8Array[i]) & 0xFF];
    return (crc ^ (-1)) >>> 0;
  }

  function createZipBlob(files) {
    const chunks = [];
    const centralHeaders = [];
    let offset = 0;
    const enc = new TextEncoder();

    for (const file of files) {
      const nameBytes = enc.encode(file.name);
      const dataBytes = file.data instanceof Uint8Array ? file.data : new Uint8Array(file.data);
      const crc = calcCrc32(dataBytes);
      const size = dataBytes.length;

      // Local Header (30 bytes + name)
      const lh = new Uint8Array(30 + nameBytes.length);
      const dvLh = new DataView(lh.buffer);
      dvLh.setUint32(0, 0x04034b50, true);
      dvLh.setUint16(4, 20, true);
      dvLh.setUint16(6, 0x0800, true); // UTF-8
      dvLh.setUint16(8, 0, true);      // Store
      dvLh.setUint16(10, 0, true);
      dvLh.setUint16(12, 0, true);
      dvLh.setUint32(14, crc, true);
      dvLh.setUint32(18, size, true);
      dvLh.setUint32(22, size, true);
      dvLh.setUint16(26, nameBytes.length, true);
      dvLh.setUint16(28, 0, true);
      lh.set(nameBytes, 30);

      chunks.push(lh);
      chunks.push(dataBytes);

      // Central Directory Header (46 bytes + name)
      const ch = new Uint8Array(46 + nameBytes.length);
      const dvCh = new DataView(ch.buffer);
      dvCh.setUint32(0, 0x02014b50, true);
      dvCh.setUint16(4, 20, true);
      dvCh.setUint16(6, 20, true);
      dvCh.setUint16(8, 0x0800, true);
      dvCh.setUint16(10, 0, true);
      dvCh.setUint16(12, 0, true);
      dvCh.setUint16(14, 0, true);
      dvCh.setUint32(16, crc, true);
      dvCh.setUint32(20, size, true);
      dvCh.setUint32(24, size, true);
      dvCh.setUint16(28, nameBytes.length, true);
      dvCh.setUint16(30, 0, true);
      dvCh.setUint16(32, 0, true);
      dvCh.setUint16(34, 0, true);
      dvCh.setUint16(36, 0, true);
      dvCh.setUint32(38, 0, true);
      dvCh.setUint32(42, offset, true);
      ch.set(nameBytes, 46);

      centralHeaders.push(ch);
      offset += lh.length + dataBytes.length;
    }

    const centralOffset = offset;
    let centralSize = 0;
    for (const ch of centralHeaders) centralSize += ch.length;

    // End of Central Directory (22 bytes)
    const eocd = new Uint8Array(22);
    const dvEo = new DataView(eocd.buffer);
    dvEo.setUint32(0, 0x06054b50, true);
    dvEo.setUint16(4, 0, true);
    dvEo.setUint16(6, 0, true);
    dvEo.setUint16(8, files.length, true);
    dvEo.setUint16(10, files.length, true);
    dvEo.setUint32(12, centralSize, true);
    dvEo.setUint32(16, centralOffset, true);
    dvEo.setUint16(20, 0, true);

    chunks.push(...centralHeaders);
    chunks.push(eocd);
    return new Blob(chunks, { type: 'application/octet-stream' });
  }

  async function parseZipArchiveAsync(uint8Array) {
    const dv = new DataView(uint8Array.buffer, uint8Array.byteOffset, uint8Array.byteLength);
    const dec = new TextDecoder();
    const results = [];
    const len = uint8Array.length;

    let offset = 0;
    while (offset + 30 <= len) {
      const sig = dv.getUint32(offset, true);
      if (sig !== 0x04034b50) break;

      const method = dv.getUint16(offset + 8, true);
      const compSize = dv.getUint32(offset + 18, true);
      const nameLen = dv.getUint16(offset + 26, true);
      const extraLen = dv.getUint16(offset + 28, true);

      const nameStart = offset + 30;
      const nameBytes = uint8Array.subarray(nameStart, nameStart + nameLen);
      const name = dec.decode(nameBytes);

      const dataStart = nameStart + nameLen + extraLen;
      let dataBytes = uint8Array.subarray(dataStart, dataStart + compSize);

      if (method === 8 && typeof DecompressionStream !== 'undefined') {
        try {
          const ds = new DecompressionStream('deflate-raw');
          const writer = ds.writable.getWriter();
          writer.write(dataBytes);
          writer.close();
          const chunks = [];
          const reader = ds.readable.getReader();
          while (true) {
            const { value, done } = await reader.read();
            if (done) break;
            chunks.push(value);
          }
          const totalL = chunks.reduce((acc, c) => acc + c.length, 0);
          const decompressed = new Uint8Array(totalL);
          let p = 0;
          for (const c of chunks) { decompressed.set(c, p); p += c.length; }
          dataBytes = decompressed;
        } catch (e) {
          console.warn('DecompressionStream error:', e);
        }
      }

      results.push({ name, method, data: dataBytes });
      offset = dataStart + compSize;
    }
    return results;
  }

  // Professional Multitrack Memory Optimizer & Trimmer:
  // Converts all multitrack stems into individual 1-channel mono console strips
  // and applies user-selected Import Time Window (Start Time & Duration)
  function optimizeAudioBuffer(audioBuf, ctx) {
    if (!audioBuf) return null;
    const numChannels = audioBuf.numberOfChannels;
    const sr = audioBuf.sampleRate || 48000;

    const cfg = state.importWindowConfig || { startMin: 0, startSec: 0, durMin: 5, durSec: 0, fullTrack: false };
    const startSec = (cfg.startMin * 60) + cfg.startSec;
    const durSec = cfg.fullTrack ? 600 : ((cfg.durMin * 60) + cfg.durSec);

    const startSample = Math.max(0, Math.min(audioBuf.length - 100, Math.floor(startSec * sr)));
    const maxSamples = Math.floor(durSec * sr);
    const len = Math.max(1, Math.min(audioBuf.length - startSample, maxSamples));

    if (numChannels <= 1 && startSample === 0 && len === audioBuf.length) {
      return audioBuf;
    }

    const monoBuf = ctx.createBuffer(1, len, sr);
    const out = monoBuf.getChannelData(0);

    if (numChannels === 1) {
      out.set(audioBuf.getChannelData(0).subarray(startSample, startSample + len));
      return monoBuf;
    }

    // 2-Channel Stereo: Intelligently sum to mono channel strip
    const c0 = audioBuf.getChannelData(0);
    const c1 = audioBuf.getChannelData(1);

    // Check if one channel is silent (e.g. Left-only or Right-only stem export)
    let peak0 = 0, peak1 = 0;
    const step = Math.max(1, Math.floor(len / 1000));
    for (let i = 0; i < len; i += step) {
      const idx = startSample + i;
      if (Math.abs(c0[idx]) > peak0) peak0 = Math.abs(c0[idx]);
      if (Math.abs(c1[idx]) > peak1) peak1 = Math.abs(c1[idx]);
    }

    if (peak0 > 0.001 && peak1 <= 0.0001) {
      // Left channel only (0 dB)
      for (let i = 0; i < len; i++) out[i] = c0[startSample + i];
    } else if (peak1 > 0.001 && peak0 <= 0.0001) {
      // Right channel only (0 dB)
      for (let i = 0; i < len; i++) out[i] = c1[startSample + i];
    } else {
      // Balanced stereo / dual-mono: sum with clean -3dB compensation
      for (let i = 0; i < len; i++) {
        out[i] = (c0[startSample + i] + c1[startSample + i]) * 0.5;
      }
    }
    return monoBuf;
  }

  function audioBufferToWav(buffer) {
    const numChannels = buffer.numberOfChannels;
    const sampleRate = buffer.sampleRate;
    const format = 1; // PCM
    const bitDepth = 16;
    const bytesPerSample = bitDepth / 8;
    const blockAlign = numChannels * bytesPerSample;
    const numSamples = buffer.length;
    const dataByteCount = numSamples * blockAlign;
    const wavBuffer = new ArrayBuffer(44 + dataByteCount);
    const view = new DataView(wavBuffer);

    function writeStr(offset, str) {
      for (let i = 0; i < str.length; i++) view.setUint8(offset + i, str.charCodeAt(i));
    }

    writeStr(0, 'RIFF');
    view.setUint32(4, 36 + dataByteCount, true);
    writeStr(8, 'WAVE');
    writeStr(12, 'fmt ');
    view.setUint32(16, 16, true);
    view.setUint16(20, format, true);
    view.setUint16(22, numChannels, true);
    view.setUint32(24, sampleRate, true);
    view.setUint32(28, sampleRate * blockAlign, true);
    view.setUint16(32, blockAlign, true);
    view.setUint16(34, bitDepth, true);
    writeStr(36, 'data');
    view.setUint32(40, dataByteCount, true);

    const channels = [];
    for (let c = 0; c < numChannels; c++) {
      channels.push(buffer.getChannelData(c));
    }
    let offset = 44;
    for (let i = 0; i < numSamples; i++) {
      for (let c = 0; c < numChannels; c++) {
        let sample = Math.max(-1, Math.min(1, channels[c][i]));
        let intSample = sample < 0 ? sample * 0x8000 : sample * 0x7FFF;
        view.setInt16(offset, intSample, true);
        offset += 2;
      }
    }
    return new Uint8Array(wavBuffer);
  }

  async function saveProjectWithAudio() {
    showToast('กำลังเตรียมไฟล์โปรเจกต์และแทร็กเสียง...');
    await new Promise(r => setTimeout(r, 40));
    try {
      const files = [];
      const trackMeta = {};

      for (let i = 0; i < kMaxTracks; i++) {
        const ch = state.channels[i];
        if (ch.buffer) {
          showToast(`กำลังเตรียมแทร็กเสียง (${i + 1}/${kMaxTracks})...`);
          await new Promise(r => setTimeout(r, 20));
          const audioBytes = audioBufferToWav(ch.buffer);
          const safeBase = (ch.name || ('Track_' + (i + 1))).replace(/[^a-zA-Z0-9_\-\u0E00-\u0E7F]/g, '_');
          const audioFileName = `track_${String(i + 1).padStart(2, '0')}_${safeBase}.wav`;
          const archivePath = `audio/${audioFileName}`;
          files.push({ name: archivePath, data: audioBytes });
          trackMeta[i] = {
            slot: i,
            path: archivePath,
            name: ch.name,
            fileName: ch.fileName || audioFileName,
            color: ch.color
          };
        }
      }

      const projData = {
        app: 'MixPracticeStudio',
        version: 2,
        savedAt: new Date().toISOString(),
        name: (state.scenes[state.selectedSceneIdx] && state.scenes[state.selectedSceneIdx].name) || 'My Mix Session',
        totalDurationSec: state.totalDurationSec,
        loopStartNorm: state.loopStartNorm,
        loopEndNorm: state.loopEndNorm,
        main: {
          faderDb: state.main.faderDb,
          mute: state.main.mute
        },
        channelPairLinked: [...state.channelPairLinked],
        channels: state.channels.map(c => ({
          index: c.index,
          name: c.name,
          color: c.color,
          linked: c.linked,
          trimDb: c.trimDb,
          phaseInvert: c.phaseInvert,
          phantom48v: c.phantom48v,
          delayOn: c.delayOn,
          delayMs: c.delayMs,
          hpfOn: c.hpfOn,
          hpfFreq: c.hpfFreq,
          gateOn: c.gateOn,
          gateThreshDb: c.gateThreshDb,
          gateRangeDb: c.gateRangeDb,
          gateAttackMs: c.gateAttackMs,
          gateHoldMs: c.gateHoldMs,
          gateReleaseMs: c.gateReleaseMs,
          gateFiltFreq: c.gateFiltFreq,
          compOn: c.compOn,
          compThreshDb: c.compThreshDb,
          compRatio: c.compRatio,
          compMakeupDb: c.compMakeupDb,
          compKneeDb: c.compKneeDb,
          compAttackMs: c.compAttackMs,
          compHoldMs: c.compHoldMs,
          compReleaseMs: c.compReleaseMs,
          compMixPct: c.compMixPct,
          eqOn: c.eqOn,
          eqLowType: c.eqLowType || 'lowshelf', eqLowFreq: c.eqLowFreq, eqLowGainDb: c.eqLowGainDb, eqLowQ: c.eqLowQ !== undefined ? c.eqLowQ : 1.0,
          eqMid1Type: c.eqMid1Type || 'peq', eqMid1Freq: c.eqMid1Freq, eqMid1GainDb: c.eqMid1GainDb, eqMid1Q: c.eqMid1Q !== undefined ? c.eqMid1Q : 1.4,
          eqMid2Type: c.eqMid2Type || 'peq', eqMid2Freq: c.eqMid2Freq, eqMid2GainDb: c.eqMid2GainDb, eqMid2Q: c.eqMid2Q !== undefined ? c.eqMid2Q : 1.4,
          eqHighType: c.eqHighType || 'highshelf', eqHighFreq: c.eqHighFreq, eqHighGainDb: c.eqHighGainDb, eqHighQ: c.eqHighQ !== undefined ? c.eqHighQ : 1.0,
          pan: c.pan,
          faderDb: c.faderDb,
          mute: c.mute,
          solo: c.solo,
          sendsDb: [...c.sendsDb],
          sendsMute: [...c.sendsMute]
        })),
        trackMeta,
        scenes: state.scenes.map(s => ({ name: s.name, snapshot: s.snapshot }))
      };

      const enc = new TextEncoder();
      files.unshift({ name: 'project.json', data: enc.encode(JSON.stringify(projData, null, 2)) });

      showToast('กำลังบีบอัดไฟล์และสร้างลิงก์ดาวน์โหลด...');
      await new Promise(r => setTimeout(r, 40));

      const blob = createZipBlob(files);
      const url = URL.createObjectURL(blob);
      const a = document.createElement('a');
      const timeStr = new Date().toISOString().slice(0, 10).replace(/-/g, '');
      const filename = `MixPractice_Project_${timeStr}.mpsproj`;
      a.href = url;
      a.download = filename;
      document.body.appendChild(a);
      a.click();
      document.body.removeChild(a);
      setTimeout(() => URL.revokeObjectURL(url), 20000);

      const stemMsg = files.length > 1 ? ` พร้อมไฟล์เสียง ${files.length - 1} แทร็ก` : ' (เฉพาะการตั้งค่า)';
      showToast(`บันทึกไฟล์ "${filename}"${stemMsg} ลงในโฟลเดอร์ดาวน์โหลดแล้ว!`);
    } catch (err) {
      console.error('Save project error:', err);
      showToast('เกิดข้อผิดพลาดในการบันทึกโปรเจกต์: ' + err.message);
    }
  }

  async function loadProjectFile(file) {
    if (!file) return;
    const ctx = ensureAudioEngine();
    showToast('กำลังอ่านไฟล์โปรเจกต์...');
    await new Promise(r => setTimeout(r, 40));
    try {
      const buffer = await file.arrayBuffer();
      const u8 = new Uint8Array(buffer);
      const isZip = u8.length > 4 && u8[0] === 0x50 && u8[1] === 0x4B && u8[2] === 0x03 && u8[3] === 0x04;

      if (isZip) {
        showToast('กำลังแตกไฟล์และอ่านข้อมูลโปรเจกต์ (.mpsproj)...');
        await new Promise(r => setTimeout(r, 40));
        const zipEntries = await parseZipArchiveAsync(u8);

        // 1. Locate project.json
        const projEntry = zipEntries.find(e => e.name.toLowerCase().endsWith('.json') || e.name === 'project.json');
        if (!projEntry) {
          throw new Error('ไม่พบข้อมูลการตั้งค่า project.json ในโปรเจกต์');
        }

        const jsonTxt = new TextDecoder().decode(projEntry.data);
        const proj = JSON.parse(jsonTxt);

        stopSourcesOnly();
        state.isPlaying = false;
        updateTransportUi();

        // 2. Restore Mixer Settings
        applySceneSnapshot(proj);
        if (proj.scenes && Array.isArray(proj.scenes)) {
          state.scenes = proj.scenes;
        }
        if (proj.channelPairLinked) {
          state.channelPairLinked = [...proj.channelPairLinked];
          for (let i = 0; i < kMaxTracks; i++) {
            state.channels[i].linked = isChannelLinked(i);
          }
        }

        // 3. Reset existing track audio
        for (let i = 0; i < kMaxTracks; i++) {
          state.channels[i].buffer = null;
          state.channels[i].fileName = '';
        }

        // 4. Decode all audio stems from archive with progressive yield
        let decodedCount = 0;
        const trackMeta = proj.trackMeta || {};

        for (let i = 0; i < kMaxTracks; i++) {
          let audioEntry = null;
          if (trackMeta[i] && trackMeta[i].path) {
            audioEntry = zipEntries.find(e => e.name === trackMeta[i].path || e.name.endsWith('/' + trackMeta[i].path.split('/').pop()));
          }
          if (!audioEntry) {
            const prefix = `track_${String(i + 1).padStart(2, '0')}_`;
            audioEntry = zipEntries.find(e => {
              const base = e.name.split('/').pop();
              return base.startsWith(prefix) || base.startsWith(String(i + 1).padStart(2, '0') + '_');
            });
          }
          if (audioEntry && audioEntry.data && audioEntry.data.length > 0) {
            showToast(`กำลังโหลดแทร็กเสียง (${decodedCount + 1})...`);
            await new Promise(r => setTimeout(r, 40));
            try {
              const rawBytes = audioEntry.data;
              const sliceBuf = rawBytes.buffer.slice(rawBytes.byteOffset, rawBytes.byteOffset + rawBytes.byteLength);
              let audioBuf = await ctx.decodeAudioData(sliceBuf);
              audioBuf = optimizeAudioBuffer(audioBuf, ctx);
              state.channels[i].buffer = audioBuf;
              state.channels[i].fileName = audioEntry.name.split('/').pop();
              if (trackMeta[i] && trackMeta[i].name) {
                state.channels[i].name = trackMeta[i].name;
              }
              decodedCount++;
            } catch (decErr) {
              console.warn('Decode stem error for Ch ' + (i + 1), decErr);
            }
            await new Promise(r => setTimeout(r, 20));
          }
        }

        state.totalDurationSec = Math.max(1, ...state.channels.map(c => c.buffer ? c.buffer.duration : 0));
        if (proj.loopStartNorm !== undefined) state.loopStartNorm = proj.loopStartNorm;
        if (proj.loopEndNorm !== undefined) state.loopEndNorm = proj.loopEndNorm;

        syncAllAudioParams();
        renderCurrentScreen();
        showToast(`โหลดโปรเจกต์สำเร็จ! นำเข้าเสียง ${decodedCount} แทร็ก พร้อมเล่นทันที`);
      } else {
        // JSON Settings Only
        const txt = new TextDecoder().decode(u8);
        const json = JSON.parse(txt);
        applySceneSnapshot(json);
        syncAllAudioParams();
        renderCurrentScreen();
        showToast('โหลดการตั้งค่าโปรเจกต์ (.json) เรียบร้อย');
      }
    } catch (err) {
      console.error('Load project error:', err);
      showToast('เกิดข้อผิดพลาดในการโหลดโปรเจกต์: ' + err.message);
    }
  }

  // ============================================================================
  // SCREEN 9: SETUP SCREEN (Matches SetupScreen.cpp + iOS PWA Add to Home Screen)
  // ============================================================================
  function renderSetupScreen() {
    const screen = document.getElementById('screen-setup');
    if (!screen) return;
    screen.innerHTML = '';

    const wrap = document.createElement('div');
    wrap.style.cssText = 'flex:1;display:grid;grid-template-columns:1.1fr 1fr;gap:10px;min-height:0;';

    const sr = state.audioCtx ? (state.audioCtx.sampleRate / 1000).toFixed(1) + ' kHz' : '48.0 kHz (Ready)';
    const leftCard = document.createElement('div');
    leftCard.className = 'detail-sub-box';
    leftCard.style.padding = '12px';
    leftCard.style.gap = '8px';
    leftCard.innerHTML = `
      <div style="display:flex;justify-content:space-between;align-items:center;">
        <span style="font-size:13.5px;font-weight:800;color:#e6e8eb;">Audio Hardware & Output Device Setup</span>
        <button class="cbtn" style="width:74px;height:24px;color:#00d2ff;" onclick="MPS.showScreen(0)">&lt; BACK</button>
      </div>
      <div style="font-size:11.5px;color:#00d2ff;font-weight:700;">
        Active Engine: Web Audio Hardware Acceleration | Sample Rate: ${sr} | 16 Tracks / 4 FX Slots
      </div>
      <div style="display:grid;grid-template-columns:1fr 1fr;gap:8px;margin-top:4px;">
        <button class="cbtn" style="height:32px;" onclick="MPS.loadDemoStems()">🔊 LOAD 8-STEM DEMO AUDIO</button>
        <button class="cbtn" style="height:32px;" onclick="MPS.playTestTone()">🔔 PLAY 1 kHz TEST TONE</button>
      </div>
      <div style="margin-top:auto;background:#14171c;border:1px solid #2b323e;border-radius:4px;padding:8px;font-size:11px;color:#b0bec5;line-height:1.45;">
        💡 <b>คำแนะนำสำหรับ iPhone / iPad (iOS Safari):</b><br>
        1. ตรวจสอบว่าไม่ได้ปิดสวิตช์เสียง (Silent Mode) ข้างตัวเครื่อง<br>
        2. แตะปุ่มแชร์ (⎋) ใน Safari แล้วเลือก <b>"เพิ่มไปยังหน้าจอโฮม" (Add to Home Screen)</b> เพื่อเปิดใช้งานแบบเต็มจอ 100% เสมือนแอปเนทีฟ
        <button class="cbtn active-cyan" style="height:26px;margin-top:6px;width:100%;font-size:11px;font-weight:700;" onclick="MPS.showPinToHomeScreenModal(true)">📲 ดูวิธีปักหมุดไว้ที่หน้าจอโฮม (Add to Home Screen)</button>
      </div>
    `;
    wrap.appendChild(leftCard);

    const rightCard = document.createElement('div');
    rightCard.className = 'detail-sub-box';
    rightCard.style.padding = '12px';
    rightCard.style.gap = '8px';
    rightCard.innerHTML = `
      <div style="font-size:13.5px;font-weight:800;color:#ffc107;">Behringer X32 / X32 Rack Hardware OSC Remote</div>
      <div style="display:grid;grid-template-columns:120px 1fr;gap:6px;align-items:center;font-size:11.5px;">
        <span style="color:#8b949e;">Console IP Address:</span>
        <input type="text" value="192.168.1.100" style="height:26px;background:#14171c;border:1px solid #323842;color:#fff;padding:0 8px;border-radius:3px;" />
        <span style="color:#8b949e;">Port (UDP):</span>
        <input type="text" value="10023" style="height:26px;background:#14171c;border:1px solid #323842;color:#fff;padding:0 8px;border-radius:3px;" />
      </div>
      <div style="font-size:11.5px;font-weight:700;color:#e53935;margin-top:4px;">
        Status: OFFLINE (Virtual Practice Lite In-Engine DSP Mode Active)
      </div>
      <div style="margin-top:auto;display:flex;gap:8px;">
        <button class="cbtn" style="flex:1;height:28px;" onclick="MPS.showTermsDialog()">Terms & Developer Info</button>
      </div>
    `;
    wrap.appendChild(rightCard);
    screen.appendChild(wrap);
  }

  // ============================================================================
  // BOTTOM INTERACTIVE TIMELINE RENDERER (Matches TimelineRangeComponent.cpp)
  // ============================================================================
  function drawMainTimeline() {
    const canvas = document.getElementById('mainTimelineCanvas');
    if (!canvas) return;
    const dpr = window.devicePixelRatio || 1;
    const w = canvas.clientWidth || 1000;
    const h = canvas.clientHeight || 30;
    if (canvas.width !== Math.round(w * dpr) || canvas.height !== Math.round(h * dpr)) {
      canvas.width = Math.round(w * dpr);
      canvas.height = Math.round(h * dpr);
    }
    const g = canvas.getContext('2d');
    g.save();
    g.scale(dpr, dpr);

    // Recessed timeline track
    g.fillStyle = '#0d1015';
    g.fillRect(0, 0, w, h);
    g.strokeStyle = '#28303d';
    g.strokeRect(0.5, 0.5, w - 1, h - 1);

    const visibleSpan = 1.0 / Math.max(1.0, state.timelineZoom);
    const normToX = (n) => ((n - state.timelineOffset) / visibleSpan) * w;

    // 1. Draw Real Waveform Peaks on Canvas (peaks.dat ~200KB / Peak Buffer)
    if (state.waveformPeaks && state.waveformPeaks.length > 0) {
      const numBins = Math.floor(state.waveformPeaks.length / 2);
      const centerY = h * 0.5;
      const waveAmp = (h * 0.44);

      g.beginPath();
      let first = true;
      for (let px = 0; px < w; px++) {
        const norm = state.timelineOffset + (px / w) * visibleSpan;
        if (norm < 0 || norm > 1.0) continue;
        const bin = Math.min(numBins - 1, Math.max(0, Math.floor(norm * numBins)));
        const maxVal = Math.max(0, state.waveformPeaks[bin * 2 + 1] || 0);
        const yTop = centerY - (maxVal * waveAmp);
        if (first) { g.moveTo(px, yTop); first = false; }
        else { g.lineTo(px, yTop); }
      }
      for (let px = w - 1; px >= 0; px--) {
        const norm = state.timelineOffset + (px / w) * visibleSpan;
        if (norm < 0 || norm > 1.0) continue;
        const bin = Math.min(numBins - 1, Math.max(0, Math.floor(norm * numBins)));
        const minVal = Math.min(0, state.waveformPeaks[bin * 2] || 0);
        const yBot = centerY - (minVal * waveAmp);
        g.lineTo(px, yBot);
      }
      g.closePath();

      // Glowing Cyan-Blue Studio Gradient Fill
      const grad = g.createLinearGradient(0, 0, 0, h);
      grad.addColorStop(0, 'rgba(0, 229, 255, 0.7)');
      grad.addColorStop(0.5, 'rgba(37, 99, 235, 0.5)');
      grad.addColorStop(1, 'rgba(0, 229, 255, 0.7)');
      g.fillStyle = grad;
      g.fill();

      // Zero-crossing center line
      g.strokeStyle = 'rgba(255, 255, 255, 0.12)';
      g.lineWidth = 1;
      g.beginPath(); g.moveTo(0, centerY); g.lineTo(w, centerY); g.stroke();
    }

    // 2. Active Streaming Buffer Window (15-30s iOS Lookahead Zone)
    if (state.totalDurationSec > 0) {
      const curSec = getCurrentPlayheadSec();
      const lookaheadSec = state.bufferLookaheadSec || 25.0;
      const bStartNorm = curSec / state.totalDurationSec;
      const bEndNorm = Math.min(1.0, (curSec + lookaheadSec) / state.totalDurationSec);
      const bStartX = Math.max(0, Math.min(w, normToX(bStartNorm)));
      const bEndX = Math.max(0, Math.min(w, normToX(bEndNorm)));

      if (bEndX > bStartX) {
        g.fillStyle = 'rgba(0, 229, 255, 0.12)';
        g.fillRect(bStartX, 0, bEndX - bStartX, h);
        g.strokeStyle = 'rgba(0, 229, 255, 0.35)';
        g.lineWidth = 1.0;
        g.strokeRect(bStartX, 0.5, bEndX - bStartX, h - 1);

        if (bEndX - bStartX > 65) {
          g.fillStyle = '#00e5ff';
          g.font = 'bold 7.5px JetBrains Mono, monospace';
          g.textAlign = 'left';
          g.fillText('BUFFER 25s', bStartX + 4, 8);
        }
      }
    }

    // Subtle ruler ticks
    g.strokeStyle = '#1b222c';
    for (let i = 1; i < 20; i++) {
      const tx = (w / 20) * i;
      g.beginPath(); g.moveTo(tx, 0); g.lineTo(tx, h); g.stroke();
    }

    // Loop Range Box [A .. B]
    const ax = Math.max(0, Math.min(w, normToX(state.loopStartNorm)));
    const bx = Math.max(0, Math.min(w, normToX(state.loopEndNorm)));
    if (bx > ax) {
      g.fillStyle = state.isLooping ? 'rgba(0, 210, 255, 0.14)' : 'rgba(255, 183, 77, 0.08)';
      g.fillRect(ax, 1, bx - ax, h - 2);
      g.strokeStyle = state.isLooping ? '#00d2ff' : '#ffb74d';
      g.lineWidth = 1.6;
      g.strokeRect(ax + 1, 1.5, Math.max(2, bx - ax - 2), h - 3);

      // Left 'A' and Right 'B' handle flags
      g.fillStyle = '#00d2ff';
      g.fillRect(ax, 1, 11, 12);
      g.fillRect(bx - 11, 1, 11, 12);
      g.fillStyle = '#000';
      g.font = 'bold 8.5px Inter, sans-serif';
      g.textAlign = 'center';
      g.fillText('A', ax + 5.5, 10);
      g.fillText('B', bx - 5.5, 10);
    }

    // Playhead Needle
    const curSec = getCurrentPlayheadSec();
    const phNorm = state.totalDurationSec > 0 ? (curSec / state.totalDurationSec) : 0;
    const px = normToX(phNorm);
    if (px >= 0 && px <= w) {
      g.strokeStyle = '#ffffff';
      g.lineWidth = 2.0;
      g.beginPath(); g.moveTo(px, 0); g.lineTo(px, h); g.stroke();

      g.fillStyle = '#00e5ff';
      g.beginPath();
      g.moveTo(px - 5, 0); g.lineTo(px + 5, 0); g.lineTo(px, 6);
      g.closePath();
      g.fill();
    }

    g.restore();
  }

  function setupTimelineInteraction() {
    const canvas = document.getElementById('mainTimelineCanvas');
    if (!canvas) return;

    let dragMode = 'seek'; // 'seek', 'a', 'b'
    const xToSongNorm = (normX) => {
      const visibleSpan = 1.0 / Math.max(1.0, state.timelineZoom);
      return Math.max(0, Math.min(1, state.timelineOffset + normX * visibleSpan));
    };

    bindDrag(canvas,
      (coords) => {
        const n = xToSongNorm(coords.x);
        if (Math.abs(n - state.loopStartNorm) < 0.025) dragMode = 'a';
        else if (Math.abs(n - state.loopEndNorm) < 0.025) dragMode = 'b';
        else {
          dragMode = 'seek';
          MPS.seekNormalized(n);
        }
      },
      (coords) => {
        const n = xToSongNorm(coords.x);
        if (dragMode === 'a') {
          state.loopStartNorm = Math.max(0, Math.min(state.loopEndNorm - 0.02, n));
        } else if (dragMode === 'b') {
          state.loopEndNorm = Math.min(1, Math.max(state.loopStartNorm + 0.02, n));
        } else {
          MPS.seekNormalized(n);
        }
      }
    );
  }

  // ============================================================================
  // REAL-TIME 30 FPS METER & UI ANIMATION LOOP
  // ============================================================================
  function startRealtimeUiLoop() {
    const timeBuf = new Uint8Array(128);
    const timeBufR = new Uint8Array(128);

    function step() {
      const decay = 1.4;

      // 1. Update Channel Peak Meters
      state.channels.forEach((ch, idx) => {
        let rawPeak = 0;
        if (state.isPlaying && ch.nodes && ch.nodes.analyser) {
          ch.nodes.analyser.getByteTimeDomainData(timeBuf);
          for (let i = 0; i < timeBuf.length; i++) {
            const v = Math.abs((timeBuf[i] - 128) / 128.0);
            if (v > rawPeak) rawPeak = v;
          }
        }
        const db = gainToDb(rawPeak);
        if (db >= 0.0) ch.clip = true;
        if (db > ch.peakDb) ch.peakDb = db;
        else ch.peakDb = Math.max(-90.0, ch.peakDb - decay);

        if (ch.peakDb >= ch.holdDb) { ch.holdDb = ch.peakDb; ch.holdTimer = 35; }
        else if (--ch.holdTimer <= 0) { ch.holdDb = Math.max(-90.0, ch.holdDb - decay * 0.6); }

        if (state.currentScreen === 0) {
          const mc = document.getElementById(`homeMeterCanvas-${idx}`);
          if (mc) drawLedMeterCanvas(mc, ch.peakDb, ch.holdDb, ch.peakDb, ch.holdDb, false, ch.clip);
        }
      });

      // 2. Update Master Stereo Meter
      let mPeakL = 0, mPeakR = 0;
      if (state.isPlaying && state.masterAnalyserL && state.masterAnalyserR) {
        state.masterAnalyserL.getByteTimeDomainData(timeBuf);
        state.masterAnalyserR.getByteTimeDomainData(timeBufR);
        for (let i = 0; i < timeBuf.length; i++) {
          const vl = Math.abs((timeBuf[i] - 128) / 128.0);
          const vr = Math.abs((timeBufR[i] - 128) / 128.0);
          if (vl > mPeakL) mPeakL = vl;
          if (vr > mPeakR) mPeakR = vr;
        }
      }
      const dbL = gainToDb(mPeakL), dbR = gainToDb(mPeakR);
      if (dbL >= 0 || dbR >= 0) state.main.clip = true;
      state.main.peakL = dbL > state.main.peakL ? dbL : Math.max(-90, state.main.peakL - decay);
      state.main.peakR = dbR > state.main.peakR ? dbR : Math.max(-90, state.main.peakR - decay);
      if (state.main.peakL >= state.main.holdL) { state.main.holdL = state.main.peakL; state.main.holdTimerL = 35; }
      else if (--state.main.holdTimerL <= 0) state.main.holdL = Math.max(-90, state.main.holdL - decay * 0.6);
      if (state.main.peakR >= state.main.holdR) { state.main.holdR = state.main.peakR; state.main.holdTimerR = 35; }
      else if (--state.main.holdTimerR <= 0) state.main.holdR = Math.max(-90, state.main.holdR - decay * 0.6);

      const masterMc = document.getElementById('masterMeterCanvas');
      if (masterMc) {
        drawLedMeterCanvas(masterMc, state.main.peakL, state.main.holdL, state.main.peakR, state.main.holdR, true, state.main.clip);
      }

      // 3. Update Detail Screen Meters, EQ Graph & Real-time GR Meter
      if (state.currentScreen === 1) {
        const selCh = state.channels[state.selectedChannel];
        const dMc = document.getElementById('detLeftMeterCanvas');
        if (dMc && selCh) drawLedMeterCanvas(dMc, selCh.peakDb, selCh.holdDb, selCh.peakDb, selCh.holdDb, false, selCh.clip);
        const eqC = document.getElementById('interactiveEqCanvas');
        if (eqC && selCh) drawInteractiveEqCanvas(eqC, selCh);

        // Real-time Gain Reduction (GR) Meter in GATE / DYN Tab
        const grBar = document.getElementById('grBarFill');
        if (grBar && selCh) {
          let grDb = 0;
          if (state.detailSubTab === 1) { // GATE
            grDb = selCh.gateOn ? (selCh.gateGrDb || 0) : 0;
          } else if (state.detailSubTab === 2) { // DYN
            if (selCh.compOn && selCh.nodes && selCh.nodes.comp) {
              const r = selCh.nodes.comp.reduction;
              const rawR = typeof r === 'number' ? r : (r && typeof r.value === 'number' ? r.value : 0);
              if (Math.abs(rawR) > 0.05) {
                grDb = rawR;
              } else if (selCh.peakDb > selCh.compThreshDb) {
                // Envelope follower calculation fallback for Safari WebKit
                const overDb = selCh.peakDb - selCh.compThreshDb;
                const ratio = Math.max(1.1, selCh.compRatio);
                grDb = -(overDb * (1.0 - 1.0 / ratio));
              }
            }
          }
          const absGr = Math.min(24, Math.abs(grDb));
          const pct = Math.min(100, (absGr / 24.0) * 100);
          grBar.style.height = pct.toFixed(1) + '%';
          const grNum = document.getElementById('grNumVal');
          if (grNum) {
            grNum.textContent = absGr >= 0.5 ? `-${Math.round(absGr)}` : '0';
          }
        }
      }

      // 4. Update Effects Screen In/Out Meters & Stereo Infinity Box
      if (state.currentScreen === 3) {
        const fxC = document.getElementById('fxInOutMetersCanvas');
        if (fxC) drawFxRoutingMetersCanvas(fxC);
      }

      // 5. Update Bank Selector Miniature 8x6 LED Matrices
      updateBankMiniMeters();

      // 6. Update Bottom Transport Timeline & Timecode Readouts
      drawMainTimeline();
      const curSec = getCurrentPlayheadSec();
      const totSec = state.totalDurationSec;
      const elTime = document.getElementById('lblTransTime');
      const elRange = document.getElementById('lblTransRange');
      if (elTime) elTime.textContent = `${formatTimeSec(curSec)} / ${formatTimeSec(totSec)}`;
      if (elRange) {
        if (totSec > 0) {
          const aSec = state.loopStartNorm * totSec;
          const bSec = state.loopEndNorm * totSec;
          elRange.textContent = `[${formatTimeSec(aSec)} - ${formatTimeSec(bSec)}] (${formatTimeSec(bSec - aSec)})`;
        } else {
          elRange.textContent = 'No Tracks Loaded';
        }
      }

      const elStatus = document.getElementById('lblTransStatus');
      if (elStatus) {
        if (totSec > 0) {
          elStatus.textContent = 'Out: WebAudio | iOS RAM Buffer: 25s (Safe) | 16-Trk Sync';
        } else {
          elStatus.textContent = 'Out: WebAudio  Buf: 100%';
        }
      }

      requestAnimationFrame(step);
    }
    requestAnimationFrame(step);
  }

  function updateBankMiniMeters() {
    const segThresholds = [0, -6, -12, -18, -30, -50];
    for (let b = 0; b < 10; b++) {
      const c = document.getElementById(`bankMeterCanvas-${b}`);
      if (!c) continue;
      const w = c.clientWidth || 68;
      const h = c.clientHeight || 19;
      if (c.width !== w || c.height !== h) { c.width = w; c.height = h; }
      const g = c.getContext('2d');
      g.clearRect(0, 0, w, h);

      const colW = w / 8.0;
      const segStep = h / 6.0;
      for (let col = 0; col < 8; col++) {
        let chDb = -90;
        if (b === 0 || b === 1) {
          chDb = state.channels[b * 8 + col].peakDb;
        } else if (b === 8 && col === 7) {
          chDb = Math.max(state.main.peakL, state.main.peakR);
        }
        const mx = col * colW;
        for (let s = 0; s < 6; s++) {
          const lit = chDb >= segThresholds[s];
          g.fillStyle = !lit ? '#1e232b' : (s === 0 ? '#ff2222' : s <= 2 ? '#ffa000' : '#00e650');
          g.fillRect(mx + 0.5, s * segStep, Math.max(2, colW - 1.5), Math.max(1.5, segStep - 1));
        }
      }
    }
  }

  function drawFxRoutingMetersCanvas(canvas) {
    const dpr = window.devicePixelRatio || 1;
    const w = canvas.clientWidth || 310;
    const h = canvas.clientHeight || 34;
    if (canvas.width !== Math.round(w * dpr) || canvas.height !== Math.round(h * dpr)) {
      canvas.width = Math.round(w * dpr);
      canvas.height = Math.round(h * dpr);
    }
    const g = canvas.getContext('2d');
    g.save();
    g.scale(dpr, dpr);
    g.clearRect(0, 0, w, h);

    const activeLvl = state.isPlaying ? (0.45 + Math.random() * 0.25) : 0;
    const draw11Seg = (x, y, lvl) => {
      g.fillStyle = '#141416';
      g.fillRect(x, y, 82, 11);
      g.strokeStyle = '#3f3f46';
      g.strokeRect(x, y, 82, 11);
      const count = Math.round(lvl * 11);
      for (let i = 0; i < 11; i++) {
        g.fillStyle = i < count ? (i >= 9 ? '#ef4444' : i >= 7 ? '#eab308' : '#22c55e') : '#1f1f23';
        g.fillRect(x + 2 + i * 7.1, y + 1.5, 5.2, 8);
      }
    };

    draw11Seg(2, 3, activeLvl);
    draw11Seg(2, 19, activeLvl * 0.92);

    // Center [ -> L -> oo -> R -> ] Box
    const bx = 92, bw = 124;
    g.fillStyle = '#252528';
    g.fillRect(bx, 1, bw, h - 2);
    g.strokeStyle = '#9ca3af';
    g.strokeRect(bx, 1, bw, h - 2);
    g.fillStyle = '#d4d4d8';
    g.font = 'bold 9px Inter, sans-serif';
    g.textAlign = 'center';
    g.fillText('→   L   →', bx + bw * 0.5, 10);
    g.fillStyle = '#38bdf8';
    g.font = 'bold 13px Inter, sans-serif';
    g.fillText('∞', bx + bw * 0.5, 21);
    g.fillStyle = '#d4d4d8';
    g.font = 'bold 9px Inter, sans-serif';
    g.fillText('→   R   →', bx + bw * 0.5, 31);

    draw11Seg(224, 3, activeLvl * 0.95);
    draw11Seg(224, 19, activeLvl * 0.95);

    g.restore();
  }

  // ============================================================================
  // POPUP MENUS, SETTINGS GEAR MENU & TERMS DIALOG
  // ============================================================================
  function openPopupMenu(items, x, y) {
    const ov = document.getElementById('mpsPopupOverlay');
    if (!ov) return;
    ov.innerHTML = '';
    ov.style.display = 'block';

    const menu = document.createElement('div');
    menu.className = 'mps-popup-menu';
    menu.style.left = Math.max(8, Math.min(state.stageW - 230, x)) + 'px';
    menu.style.top = Math.max(8, Math.min(state.stageH - 260, y)) + 'px';

    items.forEach(it => {
      if (it.header) {
        const h = document.createElement('div');
        h.className = 'mps-popup-hdr';
        h.textContent = it.header;
        menu.appendChild(h);
      } else if (it.sep) {
        const s = document.createElement('div');
        s.className = 'mps-popup-sep';
        menu.appendChild(s);
      } else {
        const row = document.createElement('div');
        row.className = 'mps-popup-item';
        row.innerHTML = `<span>${it.label}</span>${it.checked ? '<span>✓</span>' : ''}`;
        row.onclick = (e) => {
          e.stopPropagation();
          ov.style.display = 'none';
          if (it.action) it.action();
        };
        menu.appendChild(row);
      }
    });

    ov.onclick = () => { ov.style.display = 'none'; };
    ov.appendChild(menu);
  }

  function showSettingsMenu() {
    openPopupMenu([
      { header: '⚡ QUICK SHORTCUTS' },
      { label: '📲 ปักหมุดไว้ที่หน้าจอโฮม (Add to Home Screen)', action: () => showPinToHomeScreenModal(true) },
      { label: '✂️ เลือกท่อนเพลง 25% (Song Section Slicer)', action: () => openSectionSlicerModal(false) },
      { label: '📂 นำเข้าไฟล์เสียงเต็ม (Import Max 16 Tracks)', action: () => document.getElementById('hiddenAudioInput').click() },
      { sep: true },
      { header: 'SCREENS & VIEWS' },
      { label: '🏠 หน้ามิกเซอร์คอนโซล (Home Mixer)', checked: state.currentScreen === 0, action: () => showScreen(0) },
      { label: '🎛️ หน้าปรับแต่งแชนเนล (Detail: Preamp / EQ / Dyn)', checked: state.currentScreen === 1, action: () => showScreen(1) },
      { label: '🎵 หน้าจัดการแทร็กเพลง (Tracks: Import & Stems)', checked: state.currentScreen === 2, action: () => showScreen(2) },
      { label: '🎚️ หน้าเอฟเฟกต์ (Virtual Effects Rack)', checked: state.currentScreen === 3, action: () => showScreen(3) },
      { label: '📸 หน้าบันทึกซีน (Scenes & Snapshots)', checked: state.currentScreen === 4, action: () => showScreen(4) },
      { label: '⚙️ ตั้งค่าระบบเสียง (Audio & System Setup)', checked: state.currentScreen === 9, action: () => showScreen(9) },
      { sep: true },
      { header: 'PROJECT & STORAGE' },
      { label: '💾 Save Project (.mpsproj) to Downloads', action: () => MPS.saveProject() },
      { label: '📂 Open Project File (.mpsproj / .json)', action: () => MPS.loadProject() },
      { label: 'Quick Save to Browser (บันทึกลงเบราว์เซอร์)', action: () => MPS.quickSaveSession() },
      { label: 'Reload Browser Session (โหลดงานจากเบราว์เซอร์)', action: () => MPS.reloadSavedSession() },
      { sep: true },
      { label: 'Terms & Developer Info (เงื่อนไขและข้อมูลผู้พัฒนา)', action: () => MPS.showTermsDialog() },
      { label: state.showTopNavBar ? 'Hide Top Menu Bar (ซ่อนแถบเมนู)' : 'Show Top Menu Bar (แสดงแถบเมนู)', checked: state.showTopNavBar, action: () => {
        state.showTopNavBar = !state.showTopNavBar;
        renderTopNav();
        renderCurrentScreen();
      }}
    ], state.stageW - 245, 38);
  }

  function showPinToHomeScreenModal(forceShow = false) {
    const ov = document.getElementById('mpsModalOverlay');
    if (!ov) return;

    const isStandalone = isRunningStandalone();
    if (!forceShow) {
      if (isStandalone) {
        localStorage.setItem('mps_lite_pin_prompt_done', 'true');
        return;
      }
      if (localStorage.getItem('mps_lite_pin_prompt_done') === 'true') {
        return;
      }
    }

    const { isIOS, isAndroid } = detectUserPlatform();

    ov.style.display = 'flex';
    ov.innerHTML = `
      <div class="mps-modal-box allow-scroll" style="width:520px;max-width:95%;max-height:490px;overflow-y:auto;padding:16px 20px;border:1px solid #00d2ff;box-shadow:0 16px 48px rgba(0,0,0,0.9);background:linear-gradient(180deg, #171d26 0%, #0e1217 100%);">
        <!-- Header -->
        <div style="display:flex;align-items:center;justify-content:space-between;border-bottom:1px solid #253346;padding-bottom:10px;">
          <div style="display:flex;align-items:center;gap:12px;">
            <img src="/assets/app_icon.png" style="width:42px;height:42px;border-radius:10px;box-shadow:0 3px 10px rgba(0,0,0,0.6);border:1px solid rgba(0,210,255,0.4);" alt="App Icon" />
            <div>
              <div style="font-size:15px;font-weight:800;color:#00e5ff;display:flex;align-items:center;gap:6px;">
                <span>ปักหมุดไว้ที่หน้าจอโฮม</span>
                <span style="font-size:10px;background:rgba(0,210,255,0.18);color:#00d2ff;border:1px solid #00d2ff;padding:1px 6px;border-radius:4px;font-weight:700;">PWA FULLSCREEN</span>
              </div>
              <div style="font-size:11px;color:#94a3b8;">Mix Practice Studio Lite — เพิ่มไอคอนลงหน้าจอเพื่อความสะดวก</div>
            </div>
          </div>
          <button class="cbtn" style="width:28px;height:28px;font-size:14px;" id="btnClosePinModal">✕</button>
        </div>

        ${isStandalone ? `
        <!-- Already installed state -->
        <div style="background:#13241b;border:1px solid #2e7d32;border-radius:6px;padding:16px;text-align:center;margin-top:6px;">
          <div style="font-size:26px;margin-bottom:6px;">🎉</div>
          <div style="font-size:14px;font-weight:800;color:#4ade80;">แอปปักหมุดอยู่บนหน้าจอโฮมเรียบร้อยแล้ว!</div>
          <div style="font-size:11.5px;color:#cbd5e1;margin-top:6px;line-height:1.5;">
            ขณะนี้คุณกำลังใช้งาน Mix Practice Studio ในโหมด Standalone Application เต็มหน้าจอ 100% ไม่มีแถบเบราว์เซอร์กวนใจ
          </div>
        </div>
        ` : `
        <!-- Value Proposition -->
        <div style="background:rgba(0,180,216,0.08);border:1px solid rgba(0,210,255,0.22);border-radius:6px;padding:8px 12px;margin-top:4px;">
          <div style="font-size:11px;color:#cbd5e1;line-height:1.45;">
            💡 <b>ทำไมถึงควรปักหมุดไว้ที่หน้าจอ?</b>
            <div style="margin-top:3px;display:flex;flex-wrap:wrap;gap:12px;color:#e2e8f0;font-size:10.5px;">
              <span>✓ <b>เต็มจอ 100%:</b> มิกซ์เสียงได้กว้างจุใจ ไม่มีแถบ URL กั้น</span>
              <span>✓ <b>เปิดไว:</b> แตะไอคอนหน้าจอใช้งานได้ทันทีเหมือนแอปจริง</span>
              <span>✓ <b>จำค่าอัตโนมัติ:</b> เซฟค่ามิกซ์และแทร็กเพลงในเครื่อง</span>
            </div>
          </div>
        </div>

        <!-- Platform Instructions -->
        ${isIOS ? `
        <div style="background:#131922;border:1px solid #233144;border-radius:6px;padding:10px 12px;display:flex;flex-direction:column;gap:8px;margin-top:2px;">
          <div style="font-size:12px;font-weight:700;color:#38bdf8;display:flex;align-items:center;gap:6px;">
            <span>🍎 วิธีปักหมุดสำหรับ iPhone / iPad (Safari):</span>
          </div>
          
          <div style="display:flex;flex-direction:column;gap:6px;font-size:11.5px;color:#e2e8f0;">
            <div style="display:flex;align-items:center;gap:10px;background:#1a2332;padding:7px 10px;border-radius:5px;border:1px solid #2c3e55;">
              <span style="font-size:17px;min-width:24px;text-align:center;">1️⃣</span>
              <div style="line-height:1.35;">
                แตะที่ปุ่ม <b>"แชร์" (Share)</b> <span style="display:inline-block;padding:1px 6px;background:#243247;border-radius:4px;color:#38bdf8;font-size:11px;font-weight:700;">⎋ Share</span> ที่แถบเมนู Safari (ด้านล่างหรือด้านบน)
              </div>
            </div>

            <div style="display:flex;align-items:center;gap:10px;background:#1a2332;padding:7px 10px;border-radius:5px;border:1px solid #2c3e55;">
              <span style="font-size:17px;min-width:24px;text-align:center;">2️⃣</span>
              <div style="line-height:1.35;">
                เลื่อนลงมาแล้วแตะเมนู <b>"เพิ่มไปยังหน้าจอโฮม"</b> <span style="display:inline-block;padding:1px 6px;background:#243247;border-radius:4px;color:#4ade80;font-size:11px;font-weight:700;">➕ Add to Home Screen</span>
              </div>
            </div>

            <div style="display:flex;align-items:center;gap:10px;background:#1a2332;padding:7px 10px;border-radius:5px;border:1px solid #2c3e55;">
              <span style="font-size:17px;min-width:24px;text-align:center;">3️⃣</span>
              <div style="line-height:1.35;">
                แตะคำว่า <b>"เพิ่ม" (Add)</b> ที่มุมบนขวา จะมีไอคอนแอปปรากฏบนหน้าจอหลักของคุณทันที!
              </div>
            </div>
          </div>
        </div>
        ` : `
        <div style="background:#131922;border:1px solid #233144;border-radius:6px;padding:10px 12px;display:flex;flex-direction:column;gap:8px;margin-top:2px;">
          <div style="font-size:12px;font-weight:700;color:#4ade80;display:flex;align-items:center;gap:6px;">
            <span>🤖 สำหรับ Android / Google Chrome / คอมพิวเตอร์:</span>
          </div>

          <div id="pinAndroidAutoBox" style="${deferredInstallPrompt ? 'display:block;' : 'display:none;'}text-align:center;padding:10px;background:#182822;border:1px solid #2e7d32;border-radius:6px;">
            <div style="font-size:12px;color:#a5d6a7;margin-bottom:8px;">เบราว์เซอร์พร้อมติดตั้งลงหน้าจอได้ทันทีใน 1 คลิก:</div>
            <button class="cbtn active-cyan" id="btnPinPromptInstall" style="height:34px;padding:0 20px;font-size:12.5px;font-weight:800;display:inline-flex;align-items:center;gap:8px;cursor:pointer;">
              <span>📲</span> <span>กดเพื่อปักหมุดลงหน้าจอทันที (Install App)</span>
            </button>
          </div>

          <div id="pinAndroidManualGuide" style="${deferredInstallPrompt ? 'display:none;' : 'display:flex;'}flex-direction:column;gap:6px;font-size:11.5px;color:#e2e8f0;">
            <div style="display:flex;align-items:center;gap:10px;background:#1a2332;padding:7px 10px;border-radius:5px;border:1px solid #2c3e55;">
              <span style="font-size:17px;min-width:24px;text-align:center;">1️⃣</span>
              <div style="line-height:1.35;">แตะที่ปุ่มเมนู <b>3 จุด (⋮)</b> หรือไอคอนติดตั้ง ⊕ ที่แถบมุมขวาบนของเบราว์เซอร์</div>
            </div>
            <div style="display:flex;align-items:center;gap:10px;background:#1a2332;padding:7px 10px;border-radius:5px;border:1px solid #2c3e55;">
              <span style="font-size:17px;min-width:24px;text-align:center;">2️⃣</span>
              <div style="line-height:1.35;">เลือก <b>"ติดตั้งแอป" (Install app)</b> หรือ <b>"เพิ่มลงในหน้าจอหลัก" (Add to Home screen)</b></div>
            </div>
          </div>
        </div>
        `}
        `}

        <!-- Footer Buttons -->
        <div style="display:flex;align-items:center;justify-content:space-between;margin-top:6px;padding-top:10px;border-top:1px solid #253346;">
          <label style="display:flex;align-items:center;gap:6px;font-size:11px;color:#94a3b8;cursor:pointer;user-select:none;">
            <input type="checkbox" id="chkDontShowPinAgain" checked style="accent-color:#00d2ff;cursor:pointer;" />
            <span>ไม่ต้องแสดงอีกเมื่อเปิดเว็บ</span>
          </label>
          <div style="display:flex;gap:8px;">
            <button class="cbtn" style="padding:0 14px;height:28px;font-size:11.5px;" id="btnDismissPinModal">ไว้คราวหลัง</button>
            <button class="cbtn active-cyan" style="padding:0 18px;height:28px;font-size:11.5px;font-weight:700;" id="btnConfirmPinModal">เข้าใช้งานแอป ➔</button>
          </div>
        </div>
      </div>
    `;

    const finalizeDismiss = () => {
      const chk = document.getElementById('chkDontShowPinAgain');
      if (chk && chk.checked) {
        localStorage.setItem('mps_lite_pin_prompt_done', 'true');
      }
      localStorage.setItem('mps_lite_terms_ok', '1');
      ov.style.display = 'none';
    };

    const btnClose = document.getElementById('btnClosePinModal');
    if (btnClose) btnClose.onclick = finalizeDismiss;

    const btnDismiss = document.getElementById('btnDismissPinModal');
    if (btnDismiss) btnDismiss.onclick = finalizeDismiss;

    const btnConfirm = document.getElementById('btnConfirmPinModal');
    if (btnConfirm) btnConfirm.onclick = finalizeDismiss;

    const btnInstall = document.getElementById('btnPinPromptInstall');
    if (btnInstall) {
      btnInstall.onclick = async () => {
        if (deferredInstallPrompt) {
          try {
            deferredInstallPrompt.prompt();
            const choice = await deferredInstallPrompt.userChoice;
            if (choice && choice.outcome === 'accepted') {
              showToast('✓ คุณได้เลือกติดตั้งแอปเรียบร้อยแล้ว!');
              localStorage.setItem('mps_lite_pin_prompt_done', 'true');
            }
          } catch (err) {
            console.warn('Install prompt error:', err);
          } finally {
            deferredInstallPrompt = null;
            finalizeDismiss();
          }
        } else {
          finalizeDismiss();
        }
      };
    }
  }

  function showTermsDialog() {
    const ov = document.getElementById('mpsModalOverlay');
    if (!ov) return;
    ov.style.display = 'flex';
    ov.innerHTML = `
      <div class="mps-modal-box">
        <div style="font-size:14px;font-weight:800;color:#00d2ff;">เงื่อนไขการใช้งาน (Terms of Service)</div>
        <div style="font-size:11.5px;color:#e6e8eb;line-height:1.55;white-space:pre-line;">ยินดีต้อนรับสู่ Mix Practice Studio (Practice Lite)!

ข้อกำหนดและเงื่อนไขการใช้งาน:
1. แอปพลิเคชันนี้พัฒนาขึ้นเพื่อการฝึกซ้อมมิกซ์เสียงและจำลองการทำงานคอนโซลเสียง (รองรับสูงสุด 16 แทร็ก / 4 เอฟเฟกต์)
2. ผู้ใช้งานสามารถนำเข้าไฟล์เสียง Multitrack และปรับแต่งมิกซ์ได้อย่างอิสระ
3. แอปนี้ให้บริการฟรีเพื่อการศึกษาและการฝึกฝน สำหรับผู้ใช้ iOS แนะนำให้กดปุ่มแชร์ใน Safari แล้วเลือก "เพิ่มไปยังหน้าจอโฮม" (Add to Home Screen)
-----------------------------------------
ผู้พัฒนา (Developer): premsak srilarin</div>
        <div style="display:flex;justify-content:flex-end;gap:8px;margin-top:6px;">
          <button class="cbtn active-cyan" style="padding:0 16px;height:28px;" id="btnAcceptTerms">ยอมรับ (Accept)</button>
        </div>
      </div>
    `;
    document.getElementById('btnAcceptTerms').onclick = () => {
      localStorage.setItem('mps_lite_terms_ok', '1');
      ov.style.display = 'none';
    };
  }

  function openCustomLoopDialog() {
    const ov = document.getElementById('mpsModalOverlay');
    if (!ov) return;
    const totSec = state.totalDurationSec || 240;
    const curASec = state.loopStartNorm * totSec;
    const curBSec = state.loopEndNorm * totSec;

    const toMmSs = (sec) => {
      const m = Math.floor(sec / 60);
      const s = Math.floor(sec % 60);
      return `${String(m).padStart(2, '0')}:${String(s).padStart(2, '0')}`;
    };

    const parseMmSs = (str, defVal) => {
      if (!str) return defVal;
      const parts = str.split(':').map(p => parseFloat(p.trim()));
      if (parts.length === 2 && !isNaN(parts[0]) && !isNaN(parts[1])) {
        return parts[0] * 60 + parts[1];
      }
      if (parts.length === 1 && !isNaN(parts[0])) {
        return parts[0];
      }
      return defVal;
    };

    ov.style.display = 'flex';
    ov.innerHTML = `
      <div class="mps-modal-box" style="width:380px;">
        <div style="display:flex;align-items:center;justify-content:space-between;">
          <span style="font-size:14px;font-weight:800;color:#00d2ff;">⏱️ กำหนดช่วงเวลาเล่นวนซ้ำ (A-B Loop)</span>
          <button class="cbtn" style="width:26px;height:22px;" id="btnCloseLoopModal">✕</button>
        </div>
        <p style="font-size:11px;color:#8b949e;line-height:1.4;margin:0;">
          พิมพ์เวลาในรูปแบบ <b>นาที:วินาที (mm:ss)</b> ที่ต้องการให้เล่นวนซ้ำสำหรับฝึกซ้อมมิกซ์ (ความยาวเพลง: ${formatTimeSec(totSec)})
        </p>

        <div style="display:grid;grid-template-columns:1fr 1fr;gap:10px;margin-top:6px;">
          <div style="display:flex;flex-direction:column;gap:4px;">
            <label style="font-size:11px;font-weight:700;color:#00e5ff;">จุดเริ่มต้น [ A ]</label>
            <input type="text" id="inpLoopA" value="${toMmSs(curASec)}" style="background:#121417;border:1px solid #364254;border-radius:4px;color:#fff;padding:6px 8px;font-family:var(--font-mono);font-size:15px;text-align:center;" />
          </div>
          <div style="display:flex;flex-direction:column;gap:4px;">
            <label style="font-size:11px;font-weight:700;color:#ffb74d;">จุดสิ้นสุด [ B ]</label>
            <input type="text" id="inpLoopB" value="${toMmSs(curBSec)}" style="background:#121417;border:1px solid #364254;border-radius:4px;color:#fff;padding:6px 8px;font-family:var(--font-mono);font-size:15px;text-align:center;" />
          </div>
        </div>

        <div style="display:flex;gap:4px;margin-top:6px;">
          <button class="cbtn" style="flex:1;height:24px;font-size:10px;" id="btnPresetIntro">ท่อนเริ่ม (0-30s)</button>
          <button class="cbtn" style="flex:1;height:24px;font-size:10px;" id="btnPresetMid">ท่อนฮุก (กลางเพลง)</button>
          <button class="cbtn" style="flex:1;height:24px;font-size:10px;" id="btnPresetAll">ทั้งเพลง (100%)</button>
        </div>

        <div style="display:flex;justify-content:flex-end;gap:8px;margin-top:10px;">
          <button class="cbtn" style="padding:0 14px;height:28px;" id="btnCancelLoop">ยกเลิก</button>
          <button class="cbtn active-cyan" style="padding:0 18px;height:28px;font-weight:700;" id="btnApplyLoop">นำไปใช้ (Apply)</button>
        </div>
      </div>
    `;

    document.getElementById('btnCloseLoopModal').onclick = () => { ov.style.display = 'none'; };
    document.getElementById('btnCancelLoop').onclick = () => { ov.style.display = 'none'; };

    document.getElementById('btnPresetIntro').onclick = () => {
      document.getElementById('inpLoopA').value = '00:00';
      document.getElementById('inpLoopB').value = toMmSs(Math.min(30, totSec));
    };
    document.getElementById('btnPresetMid').onclick = () => {
      document.getElementById('inpLoopA').value = toMmSs(totSec * 0.35);
      document.getElementById('inpLoopB').value = toMmSs(totSec * 0.70);
    };
    document.getElementById('btnPresetAll').onclick = () => {
      document.getElementById('inpLoopA').value = '00:00';
      document.getElementById('inpLoopB').value = toMmSs(totSec);
    };

    document.getElementById('btnApplyLoop').onclick = () => {
      const aSec = parseMmSs(document.getElementById('inpLoopA').value, curASec);
      const bSec = parseMmSs(document.getElementById('inpLoopB').value, curBSec);
      const validA = Math.max(0, Math.min(totSec - 0.5, aSec));
      const validB = Math.max(validA + 0.5, Math.min(totSec, bSec));

      state.loopStartNorm = totSec > 0 ? (validA / totSec) : 0;
      state.loopEndNorm = totSec > 0 ? (validB / totSec) : 1;
      state.isLooping = true;
      ov.style.display = 'none';
      if (state.isPlaying) {
        startPlaybackFrom(validA);
      }
      updateTransportUi();
      showToast(`กำหนดช่วงเวลาฝึกซ้อม: [${formatTimeSec(validA)} - ${formatTimeSec(validB)}] เรียบร้อย`);
    };
  }

  function openImportWindowDialog() {
    const ov = document.getElementById('mpsModalOverlay');
    if (!ov) return;

    const curCfg = state.importWindowConfig || { startMin: 0, startSec: 0, durMin: 5, durSec: 0, fullTrack: false };

    ov.style.display = 'flex';
    ov.innerHTML = `
      <div class="mps-modal-box" style="width:420px;">
        <div style="display:flex;align-items:center;justify-content:space-between;">
          <span style="font-size:14px;font-weight:800;color:#00d2ff;">⏱️ เลือกช่วงเวลาของไฟล์เสียงที่จะนำเข้า (Import Time)</span>
          <button class="cbtn" style="width:26px;height:22px;" id="btnCloseImpModal">✕</button>
        </div>
        <p style="font-size:11px;color:#8b949e;line-height:1.4;margin:0;">
          กำหนดช่วงเวลาของเพลงที่จะนำเข้ามาฝึกซ้อม (ช่วยให้เลือกตัดเฉพาะท่อนที่ต้องการ และช่วยให้เบราว์เซอร์ทำงานเร็ว ไม่กิน RAM)
        </p>

        <div style="background:#13161c;border:1px solid #28303d;border-radius:4px;padding:10px;display:flex;flex-direction:column;gap:8px;margin-top:4px;">
          <div style="display:flex;align-items:center;justify-content:space-between;">
            <label style="font-size:12px;font-weight:700;color:#e6e8eb;">เวลาเริ่มต้นที่จะตัดเข้า (Start Time):</label>
            <div style="display:flex;align-items:center;gap:4px;">
              <input type="number" id="inpImpStartMin" min="0" max="60" value="${curCfg.startMin}" style="width:44px;background:#1a1d24;border:1px solid #364254;color:#fff;padding:4px;font-size:13px;text-align:center;border-radius:3px;" />
              <span style="color:#8b949e;font-size:11px;">นาที</span>
              <input type="number" id="inpImpStartSec" min="0" max="59" value="${curCfg.startSec}" style="width:44px;background:#1a1d24;border:1px solid #364254;color:#fff;padding:4px;font-size:13px;text-align:center;border-radius:3px;" />
              <span style="color:#8b949e;font-size:11px;">วินาที</span>
            </div>
          </div>

          <div style="display:flex;align-items:center;justify-content:space-between;">
            <label style="font-size:12px;font-weight:700;color:#e6e8eb;">ความยาวที่จะนำเข้า (Duration):</label>
            <div style="display:flex;align-items:center;gap:4px;">
              <input type="number" id="inpImpDurMin" min="1" max="15" value="${curCfg.durMin}" style="width:44px;background:#1a1d24;border:1px solid #364254;color:#fff;padding:4px;font-size:13px;text-align:center;border-radius:3px;" />
              <span style="color:#8b949e;font-size:11px;">นาที</span>
              <input type="number" id="inpImpDurSec" min="0" max="59" value="${curCfg.durSec}" style="width:44px;background:#1a1d24;border:1px solid #364254;color:#fff;padding:4px;font-size:13px;text-align:center;border-radius:3px;" />
              <span style="color:#8b949e;font-size:11px;">วินาที</span>
            </div>
          </div>

          <div style="display:flex;align-items:center;gap:8px;margin-top:2px;">
            <input type="checkbox" id="chkImpFull" ${curCfg.fullTrack ? 'checked' : ''} style="accent-color:#00d2ff;cursor:pointer;" />
            <label for="chkImpFull" style="font-size:11.5px;color:#94a3b8;cursor:pointer;">นำเข้าทั้งเพลง (Full Track - ไม่ตัดช่วงเวลา)</label>
          </div>
        </div>

        <div style="display:flex;gap:4px;margin-top:2px;">
          <span style="font-size:10.5px;color:#64748b;align-self:center;">ปุ่มลัด:</span>
          <button class="cbtn" style="flex:1;height:24px;font-size:10px;" id="btnPreset3m">3 นาทีแรก</button>
          <button class="cbtn" style="flex:1;height:24px;font-size:10px;" id="btnPreset4m">4 นาทีแรก</button>
          <button class="cbtn" style="flex:1;height:24px;font-size:10px;" id="btnPreset5m">5 นาทีแรก (แนะนำ)</button>
        </div>

        <div style="display:flex;justify-content:flex-end;gap:8px;margin-top:10px;">
          <button class="cbtn" style="padding:0 14px;height:28px;" id="btnCancelImp">ยกเลิก</button>
          <button class="cbtn active-cyan" style="padding:0 18px;height:28px;font-weight:700;" id="btnSaveImpWindow">บันทึกการตั้งค่า</button>
        </div>
      </div>
    `;

    document.getElementById('btnCloseImpModal').onclick = () => { ov.style.display = 'none'; };
    document.getElementById('btnCancelImp').onclick = () => { ov.style.display = 'none'; };

    document.getElementById('btnPreset3m').onclick = () => {
      document.getElementById('inpImpStartMin').value = '0';
      document.getElementById('inpImpStartSec').value = '0';
      document.getElementById('inpImpDurMin').value = '3';
      document.getElementById('inpImpDurSec').value = '0';
      document.getElementById('chkImpFull').checked = false;
    };
    document.getElementById('btnPreset4m').onclick = () => {
      document.getElementById('inpImpStartMin').value = '0';
      document.getElementById('inpImpStartSec').value = '0';
      document.getElementById('inpImpDurMin').value = '4';
      document.getElementById('inpImpDurSec').value = '0';
      document.getElementById('chkImpFull').checked = false;
    };
    document.getElementById('btnPreset5m').onclick = () => {
      document.getElementById('inpImpStartMin').value = '0';
      document.getElementById('inpImpStartSec').value = '0';
      document.getElementById('inpImpDurMin').value = '5';
      document.getElementById('inpImpDurSec').value = '0';
      document.getElementById('chkImpFull').checked = false;
    };

    document.getElementById('btnSaveImpWindow').onclick = () => {
      const sMin = parseInt(document.getElementById('inpImpStartMin').value, 10) || 0;
      const sSec = parseInt(document.getElementById('inpImpStartSec').value, 10) || 0;
      const dMin = parseInt(document.getElementById('inpImpDurMin').value, 10) || 5;
      const dSec = parseInt(document.getElementById('inpImpDurSec').value, 10) || 0;
      const full = document.getElementById('chkImpFull').checked;

      state.importWindowConfig = {
        startMin: sMin,
        startSec: sSec,
        durMin: dMin,
        durSec: dSec,
        fullTrack: full
      };
      localStorage.setItem('mps_lite_import_window', JSON.stringify(state.importWindowConfig));
      ov.style.display = 'none';
      renderTracksScreen();

      const timeMsg = full ? 'ทั้งเพลง (Full Track)' : `เริ่มที่ ${String(sMin).padStart(2,'0')}:${String(sSec).padStart(2,'0')} ความยาว ${dMin} นาที ${dSec} วิ`;
      showToast(`บันทึกช่วงเวลานำเข้า: ${timeMsg} แล้ว`);
    };
  }

  // ============================================================================
  // MULTITRACK SECTION SLICER & INDEXEDDB PERSISTENCE
  // Stores full stems in browser storage (0 RAM overhead) and slices 25% chunks (Intro, Verse, Chorus, Outro)
  // ============================================================================
  const IDB_NAME = 'MixPractice_Stems_DB';
  const IDB_STORE = 'raw_stems';

  function openStemsDB() {
    return new Promise((resolve) => {
      if (!window.indexedDB) return resolve(null);
      const req = indexedDB.open(IDB_NAME, 1);
      req.onupgradeneeded = (e) => {
        const db = e.target.result;
        if (!db.objectStoreNames.contains(IDB_STORE)) {
          db.createObjectStore(IDB_STORE, { keyPath: 'slot' });
        }
      };
      req.onsuccess = (e) => resolve(e.target.result);
      req.onerror = () => resolve(null);
    });
  }

  async function saveRawStemToDB(slot, nameOrFile, maybeFile) {
    const db = await openStemsDB();
    if (!db) return;
    try {
      const file = maybeFile || nameOrFile;
      const name = typeof nameOrFile === 'string' ? nameOrFile : file.name;
      const tx = db.transaction(IDB_STORE, 'readwrite');
      const store = tx.objectStore(IDB_STORE);
      store.put({ slot, name, blob: file });
    } catch (e) {
      console.warn('IDB put error:', e);
    }
  }

  async function getAllStoredRawStems() {
    const db = await openStemsDB();
    if (!db) return [];
    return new Promise((resolve) => {
      try {
        const tx = db.transaction(IDB_STORE, 'readonly');
        const store = tx.objectStore(IDB_STORE);
        const req = store.getAll();
        req.onsuccess = () => resolve(req.result || []);
        req.onerror = () => resolve([]);
      } catch (e) {
        resolve([]);
      }
    });
  }

  async function clearAllStoredRawStems() {
    const db = await openStemsDB();
    if (!db) return;
    try {
      const tx = db.transaction(IDB_STORE, 'readwrite');
      tx.objectStore(IDB_STORE).clear();
    } catch (e) {}
  }

  async function applyAudioSection(startPct, endPct, label) {
    const ctx = ensureAudioEngine();
    stopSourcesOnly();
    state.isPlaying = false;
    updateTransportUi();

    // 1. Ensure raw files are loaded from state or IndexedDB
    if (!state.rawFiles || !state.rawFiles.some(f => f)) {
      const stored = await getAllStoredRawStems();
      if (stored && stored.length > 0) {
        state.rawFiles = [];
        stored.forEach(item => {
          state.rawFiles[item.slot] = item.blob;
        });
      }
    }

    const availableFiles = (state.rawFiles || []).filter(f => f);
    if (!availableFiles.length) {
      showToast('ยังไม่มีไฟล์เสียงในระบบ กรุณานำเข้าไฟล์เสียงก่อน');
      return;
    }

    showToast(`กำลังเตรียมตัดเสียงท่อน "${label}"...`);
    await new Promise(r => setTimeout(r, 60));

    let processedCount = 0;
    const sPct = Math.max(0, Math.min(100, startPct));
    const ePct = Math.max(sPct + 1, Math.min(100, endPct));

    for (let i = 0; i < kMaxTracks; i++) {
      const file = state.rawFiles[i];
      if (!file) continue;

      showToast(`[${processedCount + 1}/${availableFiles.length}] กำลังตัดเสียงท่อน ${label}: ${file.name.substring(0, 20)}...`);
      await new Promise(r => setTimeout(r, 40));

      try {
        let ab = await file.arrayBuffer();
        let audioBuf = await ctx.decodeAudioData(ab);
        ab = null;

        const totalLen = audioBuf.length;
        const startSample = Math.floor(totalLen * (sPct / 100.0));
        const endSample = Math.min(totalLen, Math.floor(totalLen * (ePct / 100.0)));
        const sliceLen = Math.max(1, endSample - startSample);

        const sr = audioBuf.sampleRate || 48000;
        const monoBuf = ctx.createBuffer(1, sliceLen, sr);
        const out = monoBuf.getChannelData(0);

        const c0 = audioBuf.getChannelData(0);
        const c1 = audioBuf.numberOfChannels > 1 ? audioBuf.getChannelData(1) : c0;

        for (let s = 0; s < sliceLen; s++) {
          out[s] = (c0[startSample + s] + c1[startSample + s]) * 0.5;
        }

        audioBuf = null; // Free full buffer immediately from RAM

        state.channels[i].buffer = monoBuf;
        state.channels[i].fileName = file.name;
        state.channels[i].name = file.name.replace(/\.[^/.]+$/, '').substring(0, 10);
        state.channels[i].peaks = computeBufferPeaks(monoBuf, 800);
        processedCount++;
      } catch (err) {
        console.warn(`Error slicing track ${i + 1}:`, err);
      }
      await new Promise(r => setTimeout(r, 20));
    }

    const activePeaks = state.channels.map(c => c.peaks).filter(p => p);
    state.waveformPeaks = combinePeaks(activePeaks, 800);
    state.activeSection = { startPct: sPct, endPct: ePct, label };
    state.totalDurationSec = Math.max(1, ...state.channels.map(c => c.buffer ? c.buffer.duration : 0));
    state.loopStartNorm = 0.0;
    state.loopEndNorm = 1.0;

    syncAllAudioParams();
    drawMainTimeline();
    renderCurrentScreen();
    renderTopNav();
    showToast(`ตัดท่อน "${label}" สำเร็จ ${processedCount} แทร็ก! พร้อมเล่นทันที`);
  }

  function openSectionSlicerModal(isNewImport = false) {
    const ov = document.getElementById('mpsModalOverlay');
    if (!ov) return;

    ov.style.display = 'flex';
    ov.innerHTML = `
      <div class="mps-modal-box" style="width:480px;max-width:95%;">
        <div style="display:flex;align-items:center;justify-content:space-between;">
          <div style="display:flex;align-items:center;gap:6px;">
            <span style="font-size:18px;">✂️</span>
            <span style="font-size:15px;font-weight:800;color:#00e5ff;">เลือกท่อนเพลงที่จะนำมาเล่น (Select Mix Section)</span>
          </div>
          <button class="cbtn" style="width:28px;height:24px;" id="btnCloseSlicer">✕</button>
        </div>

        <p style="font-size:12px;color:#cbd5e1;line-height:1.5;margin:0;">
          ${isNewImport ? '<b style="color:#00e676;">✓ นำเข้าไฟล์เสียงเต็มเรียบร้อย!</b> ' : ''}เลือกว่าจะตัดเอาส่วนไหนของเพลงมาเล่นในแอป (แบ่งส่วนละ 25% หรือเล่นทั้งเพลง) สามารถสลับท่อนใหม่ได้ตลอดเวลาโดยไม่ต้องเลือกไฟล์ใหม่:
        </p>

        <div style="display:flex;flex-direction:column;gap:7px;margin-top:4px;">
          <!-- Section 1 -->
          <button class="cbtn" id="btnSec1" style="display:flex;align-items:center;justify-content:space-between;padding:10px 14px;background:#151b24;border:1px solid #2d3b4f;text-align:left;height:auto;cursor:pointer;">
            <div>
              <div style="font-size:13px;font-weight:700;color:#00e5ff;">ท่อนที่ 1 : 0% - 25% (Intro / ท่อนเริ่ม)</div>
              <div style="font-size:10.5px;color:#8b949e;">ช่วงต้นเพลง เหมาะสำหรับเช็คบาลานซ์กลอง เบส และเสียงเปิด</div>
            </div>
            <span style="font-size:12px;color:#00e5ff;font-weight:700;background:rgba(0,210,255,0.15);padding:3px 8px;border-radius:4px;">[ 0% - 25% ]</span>
          </button>

          <!-- Section 2 -->
          <button class="cbtn" id="btnSec2" style="display:flex;align-items:center;justify-content:space-between;padding:10px 14px;background:#151b24;border:1px solid #2d3b4f;text-align:left;height:auto;cursor:pointer;">
            <div>
              <div style="font-size:13px;font-weight:700;color:#00e5ff;">ท่อนที่ 2 : 25% - 50% (Verse 1 / ท่อนแรก)</div>
              <div style="font-size:10.5px;color:#8b949e;">เสียงร้องและเครื่องดนตรีเริ่มครบ เหมาะสำหรับเซ็ต Preamp, EQ และบาลานซ์เสียงร้อง</div>
            </div>
            <span style="font-size:12px;color:#00e5ff;font-weight:700;background:rgba(0,210,255,0.15);padding:3px 8px;border-radius:4px;">[ 25% - 50% ]</span>
          </button>

          <!-- Section 3 -->
          <button class="cbtn" id="btnSec3" style="display:flex;align-items:center;justify-content:space-between;padding:10px 14px;background:#182218;border:1px solid #2e7d32;text-align:left;height:auto;cursor:pointer;">
            <div>
              <div style="font-size:13px;font-weight:700;color:#4ade80;">ท่อนที่ 3 : 50% - 75% (Chorus / ท่อนฮุก - แนะนำ ✨)</div>
              <div style="font-size:10.5px;color:#a5d6a7;">ท่อนพีคพลังงานสูงสุด เครื่องดนตรีครบทุกชิ้น เหมาะสำหรับการมิกซ์หลัก</div>
            </div>
            <span style="font-size:12px;color:#4ade80;font-weight:700;background:rgba(74,222,128,0.15);padding:3px 8px;border-radius:4px;">[ 50% - 75% ]</span>
          </button>

          <!-- Section 4 -->
          <button class="cbtn" id="btnSec4" style="display:flex;align-items:center;justify-content:space-between;padding:10px 14px;background:#151b24;border:1px solid #2d3b4f;text-align:left;height:auto;cursor:pointer;">
            <div>
              <div style="font-size:13px;font-weight:700;color:#ffb74d;">ท่อนที่ 4 : 75% - 100% (Solo & Outro / ท่อนจบ)</div>
              <div style="font-size:10.5px;color:#8b949e;">ท่อนโซโล่เครื่องดนตรีและช่วงท้ายเพลง สำหรับฝึกทำ Effect Reverb & Delay ท้ายเพลง</div>
            </div>
            <span style="font-size:12px;color:#ffb74d;font-weight:700;background:rgba(255,183,77,0.15);padding:3px 8px;border-radius:4px;">[ 75% - 100% ]</span>
          </button>

          <!-- Full Song -->
          <button class="cbtn" id="btnSecFull" style="display:flex;align-items:center;justify-content:space-between;padding:9px 14px;background:#11151a;border:1px solid #252e3b;text-align:left;height:auto;cursor:pointer;">
            <div>
              <div style="font-size:12.5px;font-weight:700;color:#94a3b8;">เล่นทั้งเพลง (Full Track 100%)</div>
              <div style="font-size:10px;color:#64748b;">นำเข้าความยาวทั้งเพลงเต็ม (สำหรับเพลงทั่วไปยาว 3-4 นาที)</div>
            </div>
            <span style="font-size:11.5px;color:#94a3b8;font-weight:700;">[ 100% ]</span>
          </button>
        </div>

        <!-- Custom Percent Slicer -->
        <div style="display:flex;align-items:center;gap:8px;padding:8px 12px;background:#12151b;border:1px solid #283344;border-radius:4px;margin-top:2px;">
          <span style="font-size:11.5px;font-weight:700;color:#00e5ff;">กำหนด % เอง:</span>
          <input type="number" id="inpCustomStartPct" min="0" max="99" value="${state.activeSection.startPct || 0}" style="width:48px;background:#181d26;border:1px solid #3b495e;color:#fff;text-align:center;padding:3px;font-size:12px;border-radius:3px;" />
          <span style="color:#8b949e;font-size:11px;">% ถึง</span>
          <input type="number" id="inpCustomEndPct" min="1" max="100" value="${state.activeSection.endPct || 50}" style="width:48px;background:#181d26;border:1px solid #3b495e;color:#fff;text-align:center;padding:3px;font-size:12px;border-radius:3px;" />
          <span style="color:#8b949e;font-size:11px;">%</span>
          <button class="cbtn active-cyan" style="padding:0 12px;height:26px;font-size:11px;font-weight:700;margin-left:auto;" id="btnApplyCustomPct">ตัดเสียง</button>
        </div>

        <div style="display:flex;align-items:center;justify-content:space-between;margin-top:6px;padding-top:8px;border-top:1px solid #232a35;">
          <span style="font-size:11px;color:#64748b;">ท่อนที่ใช้งานอยู่ปัจจุบัน: <b style="color:#00e5ff;">${state.activeSection.label}</b></span>
          <button class="cbtn" style="padding:0 14px;height:26px;" id="btnCancelSlicer">ปิด</button>
        </div>
      </div>
    `;

    document.getElementById('btnCloseSlicer').onclick = () => { ov.style.display = 'none'; };
    document.getElementById('btnCancelSlicer').onclick = () => { ov.style.display = 'none'; };

    document.getElementById('btnSec1').onclick = async () => {
      ov.style.display = 'none';
      await applyAudioSection(0, 25, '0% - 25% (Intro)');
    };
    document.getElementById('btnSec2').onclick = async () => {
      ov.style.display = 'none';
      await applyAudioSection(25, 50, '25% - 50% (Verse 1)');
    };
    document.getElementById('btnSec3').onclick = async () => {
      ov.style.display = 'none';
      await applyAudioSection(50, 75, '50% - 75% (Chorus)');
    };
    document.getElementById('btnSec4').onclick = async () => {
      ov.style.display = 'none';
      await applyAudioSection(75, 100, '75% - 100% (Solo & Outro)');
    };
    document.getElementById('btnSecFull').onclick = async () => {
      ov.style.display = 'none';
      await applyAudioSection(0, 100, 'Full (100%)');
    };

    document.getElementById('btnApplyCustomPct').onclick = async () => {
      const s = parseInt(document.getElementById('inpCustomStartPct').value, 10) || 0;
      const e = parseInt(document.getElementById('inpCustomEndPct').value, 10) || 100;
      const validS = Math.max(0, Math.min(99, s));
      const validE = Math.max(validS + 1, Math.min(100, e));
      ov.style.display = 'none';
      await applyAudioSection(validS, validE, `${validS}% - ${validE}% (Custom)`);
    };
  }

  // ============================================================================
  // SCREEN SWITCHING & PUBLIC API (window.MPS)
  // ============================================================================
  function showScreen(screenIdx) {
    state.currentScreen = screenIdx;
    renderTopNav();
    document.querySelectorAll('.mps-screen').forEach(el => el.classList.remove('active'));

    const map = {
      0: 'screen-home',
      1: 'screen-detail',
      2: 'screen-tracks',
      3: 'screen-effects',
      4: 'screen-scenes',
      9: 'screen-setup'
    };
    const targetId = map[screenIdx] || 'screen-placeholder';
    const targetEl = document.getElementById(targetId);
    if (targetEl) targetEl.classList.add('active');
    renderCurrentScreen();
  }

  function renderCurrentScreen() {
    if (state.currentScreen === 0) renderHomeScreen();
    else if (state.currentScreen === 1) renderDetailScreen();
    else if (state.currentScreen === 2) renderTracksScreen();
    else if (state.currentScreen === 3) renderEffectsScreen();
    else if (state.currentScreen === 4) renderScenesScreen();
    else if (state.currentScreen === 9) renderSetupScreen();
    else {
      const ph = document.getElementById('screen-placeholder');
      if (ph) {
        const tab = kNavTabs[state.currentScreen] || { name: 'Module' };
        ph.innerHTML = `<div style="margin:auto;text-align:center;color:#8b949e;font-size:14px;font-weight:700;">${tab.name}<br><br><span style="font-size:11.5px;font-weight:500;">(Scheduled for Phase 4 Expansion as specified in architecture roadmap)</span></div>`;
      }
    }
  }

  function showToast(msg) {
    let t = document.getElementById('mpsToast');
    if (!t) {
      t = document.createElement('div');
      t.id = 'mpsToast';
      t.style.cssText = 'position:absolute;bottom:76px;left:50%;transform:translateX(-50%);background:rgba(20,24,31,0.96);border:1px solid #00d2ff;color:#fff;padding:6px 14px;border-radius:16px;font-size:11px;font-weight:700;z-index:900;pointer-events:none;transition:opacity 0.2s;';
      document.getElementById('mps-stage').appendChild(t);
    }
    t.textContent = msg;
    t.style.opacity = '1';
    clearTimeout(t._tm);
    t._tm = setTimeout(() => { t.style.opacity = '0'; }, 2400);
  }

  window.MPS = {
    showScreen,
    loadDemoStems: async () => {
      await loadDemoStems();
      showToast('โหลดเพลงฝึกซ้อม 8 แทร็กเรียบร้อย กด PLAY เพื่อเริ่มฟังเสียง!');
    },
    play: () => startPlaybackFrom(state.playheadSec),
    pause: () => {
      if (!state.isPlaying) return;
      state.playheadSec = getCurrentPlayheadSec();
      state.isPlaying = false;
      stopSourcesOnly();
      updateTransportUi();
    },
    stop: () => {
      state.isPlaying = false;
      state.playheadSec = state.isLooping ? (state.loopStartNorm * state.totalDurationSec) : 0;
      stopSourcesOnly();
      updateTransportUi();
    },
    toggleLoop: () => {
      state.isLooping = !state.isLooping;
      updateTransportUi();
      if (state.isPlaying) startPlaybackFrom(getCurrentPlayheadSec());
    },
    seekNormalized: (norm) => {
      const targetSec = Math.max(0, Math.min(1, norm)) * state.totalDurationSec;
      state.playheadSec = targetSec;
      if (state.isPlaying) startPlaybackFrom(targetSec);
    },
    setMarkerA: () => {
      if (state.totalDurationSec <= 0) return;
      const curN = getCurrentPlayheadSec() / state.totalDurationSec;
      state.loopStartNorm = Math.max(0, Math.min(state.loopEndNorm - 0.02, curN));
      if (state.isPlaying) startPlaybackFrom(getCurrentPlayheadSec());
    },
    setMarkerB: () => {
      if (state.totalDurationSec <= 0) return;
      const curN = getCurrentPlayheadSec() / state.totalDurationSec;
      state.loopEndNorm = Math.min(1, Math.max(state.loopStartNorm + 0.02, curN));
      if (state.isPlaying) startPlaybackFrom(getCurrentPlayheadSec());
    },
    setRangeAll: () => {
      state.loopStartNorm = 0.0;
      state.loopEndNorm = 1.0;
      if (state.isPlaying) startPlaybackFrom(getCurrentPlayheadSec());
    },
    zoomTimeline: (factor) => {
      state.timelineZoom = Math.max(1.0, Math.min(16.0, state.timelineZoom * factor));
      const span = 1.0 / state.timelineZoom;
      state.timelineOffset = Math.max(0, Math.min(1.0 - span, state.timelineOffset));
    },
    resetTimelineZoom: () => {
      state.timelineZoom = 1.0;
      state.timelineOffset = 0.0;
    },
    zoomToLoop: () => {
      const span = Math.max(0.05, state.loopEndNorm - state.loopStartNorm);
      state.timelineZoom = Math.min(16.0, 1.0 / span);
      state.timelineOffset = state.loopStartNorm;
    },
    openSectionMenu: () => {
      openPopupMenu([
        { header: 'SONG SECTION LOOP PRESETS' },
        { label: 'Full Song (100%)', action: () => { state.loopStartNorm = 0; state.loopEndNorm = 1; if (state.isPlaying) startPlaybackFrom(0); } },
        { sep: true },
        { label: 'Intro (0% - 25%)', action: () => { state.loopStartNorm = 0; state.loopEndNorm = 0.25; if (state.isPlaying) startPlaybackFrom(0); } },
        { label: 'Verse 1 (25% - 50%)', action: () => { state.loopStartNorm = 0.25; state.loopEndNorm = 0.50; if (state.isPlaying) startPlaybackFrom(state.totalDurationSec * 0.25); } },
        { label: 'Chorus (50% - 75%)', action: () => { state.loopStartNorm = 0.50; state.loopEndNorm = 0.75; if (state.isPlaying) startPlaybackFrom(state.totalDurationSec * 0.50); } },
        { label: 'Outro / Solo (75% - 100%)', action: () => { state.loopStartNorm = 0.75; state.loopEndNorm = 1.0; if (state.isPlaying) startPlaybackFrom(state.totalDurationSec * 0.75); } },
        { sep: true },
        { label: '⏱️ Custom Range (พิมพ์เวลาเริ่มต้น-สิ้นสุดเอง)...', action: () => openCustomLoopDialog() }
      ], 260, state.stageH - 240);
    },
    openCustomLoopDialog,
    openImportWindowDialog,
    selectEqBand: (b) => {
      state.selectedEqBand = b;
      renderDetailScreen();
    },
    setEqBandType: (bIdx, type) => {
      setEqBandType(bIdx, type);
    },
    moveTrack: (idx, dir) => {
      const target = idx + dir;
      if (target < 0 || target >= kMaxTracks) return;
      const tmp = state.channels[idx].buffer;
      const tmpName = state.channels[idx].name;
      const tmpFile = state.channels[idx].fileName;
      state.channels[idx].buffer = state.channels[target].buffer;
      state.channels[idx].name = state.channels[target].name;
      state.channels[idx].fileName = state.channels[target].fileName;
      state.channels[target].buffer = tmp;
      state.channels[target].name = tmpName;
      state.channels[target].fileName = tmpFile;
      renderTracksScreen();
    },
    clearTrackSlot: async (idx) => {
      state.channels[idx].buffer = null;
      state.channels[idx].fileName = '';
      state.channels[idx].name = 'CH ' + String(idx + 1).padStart(2, '0');
      if (state.rawFiles && state.rawFiles[idx]) {
        state.rawFiles[idx] = null;
      }
      try {
        const db = await openStemsDB();
        if (db) {
          const tx = db.transaction(IDB_STORE, 'readwrite');
          tx.objectStore(IDB_STORE).delete(idx);
        }
      } catch (e) {}
      renderTracksScreen();
      renderTopNav();
    },
    saveProject: saveProjectWithAudio,
    saveProjectWithAudio: saveProjectWithAudio,
    loadProject: () => {
      const inp = document.getElementById('hiddenSceneInput');
      if (inp) {
        inp.value = '';
        inp.click();
      }
    },
    loadProjectFile: loadProjectFile,
    quickSaveSession: () => {
      localStorage.setItem('mps_lite_saved_session', JSON.stringify(captureSnapshotData()));
      showToast('บันทึกการตั้งค่ามิกเซอร์ปัจจุบันลงในเครื่องเรียบร้อย (Saved)');
    },
    reloadSavedSession: () => {
      const raw = localStorage.getItem('mps_lite_saved_session');
      if (!raw) return showToast('ยังไม่มีงานที่บันทึกไว้');
      try {
        applySceneSnapshot(JSON.parse(raw));
        renderCurrentScreen();
        showToast('โหลดการตั้งค่าที่บันทึกไว้เรียบร้อย');
      } catch (e) {}
    },
    exportSceneJson: () => {
      const blob = new Blob([JSON.stringify(captureSnapshotData(), null, 2)], { type: 'application/json' });
      const a = document.createElement('a');
      a.href = URL.createObjectURL(blob);
      a.download = 'MixPracticeLite_Session.json';
      a.click();
    },
    playTestTone: () => {
      const ctx = ensureAudioEngine();
      const osc = ctx.createOscillator();
      const g = ctx.createGain();
      osc.frequency.value = 1000;
      g.gain.setValueAtTime(0.15, ctx.currentTime);
      g.gain.exponentialRampToValueAtTime(0.001, ctx.currentTime + 0.6);
      osc.connect(g); g.connect(ctx.destination);
      osc.start(); osc.stop(ctx.currentTime + 0.65);
      showToast('กำลังเล่นเสียงทดสอบ 1 kHz Test Tone');
    },
    showTermsDialog,
    showPinToHomeScreenModal: (force = true) => showPinToHomeScreenModal(force),
    forcePortraitPreview: () => {
      state.forcePortrait = true;
      fitStageToViewport();
      renderCurrentScreen();
    }
  };

  // ============================================================================
  // INITIALIZATION & EVENT LISTENERS
  // ============================================================================
  window.addEventListener('DOMContentLoaded', () => {
    fitStageToViewport();
    window.addEventListener('resize', () => {
      fitStageToViewport();
      renderCurrentScreen();
    });
    window.addEventListener('orientationchange', () => {
      setTimeout(() => {
        fitStageToViewport();
        renderCurrentScreen();
      }, 150);
    });
    if (window.visualViewport) {
      window.visualViewport.addEventListener('resize', () => {
        fitStageToViewport();
        renderCurrentScreen();
      });
    }

    // Block Safari native page bounce / vertical scroll globally except inside .allow-scroll
    document.addEventListener('touchmove', (e) => {
      if (!e.target.closest('.allow-scroll')) {
        if (e.cancelable) e.preventDefault();
      }
    }, { passive: false });

    // Top-right Settings Gear Button
    const gearBtn = document.getElementById('settingsGearBtn');
    if (gearBtn) gearBtn.onclick = (e) => { e.stopPropagation(); showSettingsMenu(); };

    // Audio Files Input Handler (Max 16 Tracks) with Smart Section Slicer
    const audioInp = document.getElementById('hiddenAudioInput');
    if (audioInp) {
      audioInp.onchange = async (e) => {
        const files = Array.from(e.target.files || []);
        if (!files.length) return;
        audioInp.value = '';

        // Stop active playback so CPU/RAM is 100% dedicated
        stopSourcesOnly();
        state.isPlaying = false;
        updateTransportUi();

        // Sort files naturally by filename (Track 01, Track 02...)
        files.sort((a, b) => a.name.localeCompare(b.name, undefined, { numeric: true, sensitivity: 'base' }));

        const toLoad = files.slice(0, kMaxTracks);
        showToast(`กำลังจัดเตรียมไฟล์เสียง ${toLoad.length} แทร็ก...`);

        state.rawFiles = [];
        for (let i = 0; i < toLoad.length; i++) {
          const file = toLoad[i];
          state.rawFiles[i] = file;
          await saveRawStemToDB(i, file.name, file);
          state.channels[i].fileName = file.name;
          state.channels[i].name = file.name.replace(/\.[^/.]+$/, '').substring(0, 10);

          // Asynchronous Background Peak Extraction (Worker Thread -> Canvas)
          MpsChunkStreamer.extractPeaksAsync(file, i).then(res => {
            if (res && res.peaks) {
              state.channels[i].peaks = res.peaks;
              const activePeaks = state.channels.map(c => c.peaks).filter(p => p);
              state.waveformPeaks = combinePeaks(activePeaks, 800);
              drawMainTimeline();
            }
          });
        }

        renderTopNav();
        renderCurrentScreen();
        showToast(`นำเข้าไฟล์เต็ม ${toLoad.length} แทร็กเรียบร้อย! กรุณาเลือกท่อนเพลงที่ต้องการเล่น`);

        // Automatically prompt the user to pick their 25% section or full song
        openSectionSlicerModal(true);
      };
    }

    // Project File (.mpsproj / .json) Input Handler
    const sceneInp = document.getElementById('hiddenSceneInput');
    if (sceneInp) {
      sceneInp.onchange = async (e) => {
        const f = e.target.files && e.target.files[0];
        if (!f) return;
        await loadProjectFile(f);
        sceneInp.value = '';
      };
    }

    // Drag and drop project files (.mpsproj / .json) or multitrack audio files directly into window
    window.addEventListener('dragover', (e) => { e.preventDefault(); });
    window.addEventListener('drop', async (e) => {
      e.preventDefault();
      if (e.dataTransfer && e.dataTransfer.files && e.dataTransfer.files.length > 0) {
        const fileList = Array.from(e.dataTransfer.files);
        const f0 = fileList[0];
        const lower0 = f0.name.toLowerCase();
        if (lower0.endsWith('.mpsproj') || lower0.endsWith('.zip') || lower0.endsWith('.json')) {
          await loadProjectFile(f0);
        } else {
          // Audio files dropped
          const audioFiles = fileList.filter(f => /\.(wav|mp3|m4a|aac|ogg|flac|aif|aiff)$/i.test(f.name) || (f.type && f.type.startsWith('audio/')));
          if (audioFiles.length > 0) {
            stopSourcesOnly();
            state.isPlaying = false;
            updateTransportUi();

            audioFiles.sort((a, b) => a.name.localeCompare(b.name, undefined, { numeric: true, sensitivity: 'base' }));
            const toLoad = audioFiles.slice(0, kMaxTracks);
            showToast(`กำลังจัดเตรียมไฟล์เสียง ${toLoad.length} แทร็ก...`);

            state.rawFiles = [];
            for (let i = 0; i < toLoad.length; i++) {
              const file = toLoad[i];
              state.rawFiles[i] = file;
              await saveRawStemToDB(i, file.name, file);
              state.channels[i].fileName = file.name;
              state.channels[i].name = file.name.replace(/\.[^/.]+$/, '').substring(0, 10);

              // Asynchronous Background Peak Extraction (Worker Thread -> Canvas)
              MpsChunkStreamer.extractPeaksAsync(file, i).then(res => {
                if (res && res.peaks) {
                  state.channels[i].peaks = res.peaks;
                  const activePeaks = state.channels.map(c => c.peaks).filter(p => p);
                  state.waveformPeaks = combinePeaks(activePeaks, 800);
                  drawMainTimeline();
                }
              });
            }

            renderTopNav();
            renderCurrentScreen();
            showToast(`นำเข้าไฟล์เต็ม ${toLoad.length} แทร็กเรียบร้อย! กรุณาเลือกท่อนเพลงที่ต้องการเล่น`);

            openSectionSlicerModal(true);
          }
        }
      }
    });

    // Spacebar Play/Pause
    window.addEventListener('keydown', (e) => {
      if (e.code === 'Space' && e.target.tagName !== 'INPUT') {
        e.preventDefault();
        if (state.isPlaying) MPS.pause();
        else MPS.play();
      }
    });

    // Instant synchronous auto-save on tab close / reload
    window.addEventListener('beforeunload', () => {
      try {
        localStorage.setItem('mps_lite_auto_session', JSON.stringify(captureSnapshotData()));
      } catch (e) {}
    });

    // 1. Auto-Restore Mixer Session Settings from localStorage
    const savedAutoSession = localStorage.getItem('mps_lite_auto_session');
    if (savedAutoSession) {
      try {
        const snap = JSON.parse(savedAutoSession);
        applySceneSnapshot(snap);
      } catch (e) {}
    }

    renderTopNav();
    renderHomeScreen();
    setupTimelineInteraction();

    const elRange = document.getElementById('lblTransRange');
    if (elRange) {
      elRange.style.cursor = 'pointer';
      elRange.title = 'คลิกเพื่อกำหนดช่วงเวลาเล่นวนซ้ำ (Custom Time Range)';
      elRange.onclick = () => openCustomLoopDialog();
    }

    startRealtimeUiLoop();

    // 2. Auto-Restore Audio Stems from IndexedDB & Auto-Decode Active Section
    getAllStoredRawStems().then(async (stems) => {
      if (stems && stems.length > 0) {
        state.rawFiles = [];
        stems.forEach(item => {
          state.rawFiles[item.slot] = item.blob;
          if (!state.channels[item.slot].fileName) {
            state.channels[item.slot].fileName = item.name;
            state.channels[item.slot].name = item.name.replace(/\.[^/.]+$/, '').substring(0, 10);
          }
        });

        // Automatically decode & load the user's active 25% section!
        const sec = state.activeSection || { startPct: 0, endPct: 25, label: '0% - 25% (Intro)' };
        try {
          await applyAudioSection(sec.startPct, sec.endPct, sec.label);
          showToast(`✓ เรียกคืนการตั้งค่าและแทร็กเสียงเดิมอัตโนมัติ (${sec.label}) พร้อมเล่นต่อทันที!`);
        } catch (err) {
          console.warn('Auto-restore audio slice error:', err);
        }

        renderTopNav();
        renderCurrentScreen();
      } else {
        renderTopNav();
        renderCurrentScreen();
        // First time user with no stems and no saved settings: show terms dialog if pin modal done
        if (!localStorage.getItem('mps_lite_terms_ok') && localStorage.getItem('mps_lite_pin_prompt_done')) {
          showTermsDialog();
        }
      }

      // Automatically prompt to pin to home screen if opening web app for first time & not in standalone mode
      if (!isRunningStandalone() && !localStorage.getItem('mps_lite_pin_prompt_done')) {
        setTimeout(() => showPinToHomeScreenModal(false), 350);
      }
    }).catch(() => {
      if (!isRunningStandalone() && !localStorage.getItem('mps_lite_pin_prompt_done')) {
        setTimeout(() => showPinToHomeScreenModal(false), 350);
      }
    });
  });

})();
