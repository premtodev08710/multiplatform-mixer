// ============================================================================
// Mix Practice Studio Lite — iOS & Web Background Audio Worker
// Handles: Asynchronous WAV/PCM Parsing, Chunk Extraction, and Waveform Peaks
// ============================================================================

self.onmessage = async function(e) {
  const { type, id, data } = e.data;

  try {
    if (type === 'GENERATE_PEAKS') {
      const { arrayBuffer, numPoints, trackSlot } = data;
      const result = extractWaveformPeaks(arrayBuffer, numPoints || 800);
      self.postMessage({
        type: 'PEAKS_READY',
        id,
        trackSlot,
        peaks: result.peaks,
        durationSec: result.durationSec,
        sampleRate: result.sampleRate,
        channels: result.channels
      }, [result.peaks.buffer]);
    } 
    else if (type === 'SLICE_CHUNK') {
      const { arrayBuffer, startSec, durationSec, trackSlot, chunkIdx } = data;
      const chunk = sliceWavChunk(arrayBuffer, startSec, durationSec);
      self.postMessage({
        type: 'CHUNK_READY',
        id,
        trackSlot,
        chunkIdx,
        pcmData: chunk.pcmData,
        sampleRate: chunk.sampleRate,
        channels: chunk.channels,
        durationSec: chunk.durationSec
      }, [chunk.pcmData.buffer]);
    }
  } catch (err) {
    self.postMessage({
      type: 'WORKER_ERROR',
      id,
      error: err.message || String(err)
    });
  }
};

// ----------------------------------------------------------------------------
// Fast Zero-Copy WAV Header Parser
// ----------------------------------------------------------------------------
function parseWavHeader(buffer) {
  const view = new DataView(buffer);
  if (buffer.byteLength < 44) return null;

  const magic = String.fromCharCode(view.getUint8(0), view.getUint8(1), view.getUint8(2), view.getUint8(3));
  if (magic !== 'RIFF') return null;

  let offset = 12;
  let fmt = null;
  let dataOffset = 0;
  let dataSize = 0;

  while (offset + 8 <= buffer.byteLength) {
    const chunkId = String.fromCharCode(
      view.getUint8(offset), view.getUint8(offset + 1),
      view.getUint8(offset + 2), view.getUint8(offset + 3)
    );
    const chunkSize = view.getUint32(offset + 4, true);

    if (chunkId === 'fmt ' && offset + 24 <= buffer.byteLength) {
      fmt = {
        audioFormat: view.getUint16(offset + 8, true),
        channels: view.getUint16(offset + 10, true),
        sampleRate: view.getUint32(offset + 12, true),
        byteRate: view.getUint32(offset + 16, true),
        blockAlign: view.getUint16(offset + 20, true),
        bitsPerSample: view.getUint16(offset + 22, true)
      };
    } else if (chunkId === 'data') {
      dataOffset = offset + 8;
      dataSize = Math.min(chunkSize, buffer.byteLength - dataOffset);
      break;
    }
    offset += 8 + chunkSize;
  }

  if (!fmt || dataOffset === 0) return null;
  return { fmt, dataOffset, dataSize };
}

// ----------------------------------------------------------------------------
// Waveform Peak Extraction (peaks.dat ~1.6 KB per track)
// Computes normalized min/max pairs [-1.0 .. 1.0] across numPoints
// ----------------------------------------------------------------------------
function extractWaveformPeaks(buffer, numPoints) {
  const wav = parseWavHeader(buffer);
  const points = Math.max(100, Math.min(2000, numPoints || 800));
  const peaks = new Float32Array(points * 2);

  if (!wav) {
    // Return empty fallback peaks if not WAV
    return {
      peaks,
      durationSec: 0,
      sampleRate: 48000,
      channels: 1
    };
  }

  const { fmt, dataOffset, dataSize } = wav;
  const bytesPerSample = Math.floor(fmt.bitsPerSample / 8) || 2;
  const blockAlign = fmt.blockAlign || (fmt.channels * bytesPerSample);
  const totalFrames = Math.floor(dataSize / blockAlign);
  const durationSec = totalFrames / (fmt.sampleRate || 48000);

  const view = new DataView(buffer, dataOffset, dataSize);
  const framesPerBin = Math.max(1, Math.floor(totalFrames / points));

  for (let p = 0; p < points; p++) {
    const frameStart = p * framesPerBin;
    const frameEnd = Math.min(totalFrames, frameStart + framesPerBin);

    let min = 1.0;
    let max = -1.0;

    // Subsample across the bin for speed (up to 32 samples per bin)
    const stride = Math.max(1, Math.floor((frameEnd - frameStart) / 32));

    for (let f = frameStart; f < frameEnd; f += stride) {
      const byteIdx = f * blockAlign;
      if (byteIdx + bytesPerSample > dataSize) break;

      let val = 0.0;
      if (fmt.bitsPerSample === 16) {
        val = view.getInt16(byteIdx, true) / 32768.0;
      } else if (fmt.bitsPerSample === 24) {
        const b0 = view.getUint8(byteIdx);
        const b1 = view.getUint8(byteIdx + 1);
        const b2 = view.getInt8(byteIdx + 2);
        val = ((b2 << 16) | (b1 << 8) | b0) / 8388608.0;
      } else if (fmt.bitsPerSample === 32) {
        val = (fmt.audioFormat === 3) 
          ? view.getFloat32(byteIdx, true) 
          : view.getInt32(byteIdx, true) / 2147483648.0;
      }

      if (val < min) min = val;
      if (val > max) max = val;
    }

    if (min > max) { min = 0; max = 0; }
    peaks[p * 2] = min;
    peaks[p * 2 + 1] = max;
  }

  return {
    peaks,
    durationSec,
    sampleRate: fmt.sampleRate,
    channels: fmt.channels
  };
}

// ----------------------------------------------------------------------------
// 5-Second Chunk Extraction from Raw WAV
// Converts raw bytes directly into normalized Float32 mono PCM
// ----------------------------------------------------------------------------
function sliceWavChunk(buffer, startSec, durationSec) {
  const wav = parseWavHeader(buffer);
  const targetDuration = Math.max(0.1, durationSec || 5.0);

  if (!wav) {
    return {
      pcmData: new Float32Array(Math.floor(48000 * targetDuration)),
      sampleRate: 48000,
      channels: 1,
      durationSec: targetDuration
    };
  }

  const { fmt, dataOffset, dataSize } = wav;
  const bytesPerSample = Math.floor(fmt.bitsPerSample / 8) || 2;
  const blockAlign = fmt.blockAlign || (fmt.channels * bytesPerSample);
  const totalFrames = Math.floor(dataSize / blockAlign);

  const startFrame = Math.max(0, Math.floor(startSec * fmt.sampleRate));
  const frameCount = Math.min(totalFrames - startFrame, Math.floor(targetDuration * fmt.sampleRate));

  if (frameCount <= 0) {
    return {
      pcmData: new Float32Array(1),
      sampleRate: fmt.sampleRate,
      channels: 1,
      durationSec: 0
    };
  }

  const outPcm = new Float32Array(frameCount);
  const view = new DataView(buffer, dataOffset, dataSize);

  for (let i = 0; i < frameCount; i++) {
    const byteIdx = (startFrame + i) * blockAlign;
    if (byteIdx + bytesPerSample > dataSize) break;

    let sampleL = 0;
    let sampleR = 0;

    if (fmt.bitsPerSample === 16) {
      sampleL = view.getInt16(byteIdx, true) / 32768.0;
      sampleR = (fmt.channels > 1 && byteIdx + 4 <= dataSize) 
        ? view.getInt16(byteIdx + 2, true) / 32768.0 
        : sampleL;
    } else if (fmt.bitsPerSample === 24) {
      const b0 = view.getUint8(byteIdx);
      const b1 = view.getUint8(byteIdx + 1);
      const b2 = view.getInt8(byteIdx + 2);
      sampleL = ((b2 << 16) | (b1 << 8) | b0) / 8388608.0;

      if (fmt.channels > 1 && byteIdx + 6 <= dataSize) {
        const r0 = view.getUint8(byteIdx + 3);
        const r1 = view.getUint8(byteIdx + 4);
        const r2 = view.getInt8(byteIdx + 5);
        sampleR = ((r2 << 16) | (r1 << 8) | r0) / 8388608.0;
      } else {
        sampleR = sampleL;
      }
    } else if (fmt.bitsPerSample === 32) {
      if (fmt.audioFormat === 3) {
        sampleL = view.getFloat32(byteIdx, true);
        sampleR = (fmt.channels > 1 && byteIdx + 8 <= dataSize) 
          ? view.getFloat32(byteIdx + 4, true) 
          : sampleL;
      } else {
        sampleL = view.getInt32(byteIdx, true) / 2147483648.0;
        sampleR = (fmt.channels > 1 && byteIdx + 8 <= dataSize) 
          ? view.getInt32(byteIdx + 4, true) / 2147483648.0 
          : sampleL;
      }
    }

    // Downmix to mono channel for console strip processing
    outPcm[i] = (sampleL + sampleR) * 0.5;
  }

  return {
    pcmData: outPcm,
    sampleRate: fmt.sampleRate,
    channels: 1,
    durationSec: frameCount / fmt.sampleRate
  };
}
