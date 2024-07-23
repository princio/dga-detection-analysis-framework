'use client';

import { useEffect, useMemo, useState } from 'react';
import axios from 'axios';
import PagePcap from '@/app/pcap/[id]/page';

enum Refresh {
  idle = 0,
  started = 2,
  error = 3,
}

enum NN {
  NONE = 1,
  TLD = 2,
  ICANN = 3,
  PRIVATE = 4,
}

export default function Home() {
  const [config, setConfig] = useState<SlotConfig>({
    sps: 1,
    onlyfirsts: 'global',
    nn: 1,
    th: 0.999,
    packet_type: 'nx',
    range: [0, 10],
  });
  const [svg, setSVG] = useState<string>('');
  const [plotting, setPlotting] = useState<boolean>(false);
  const [sps_u, setSPSU] = useState<number>(3600);
  const [pcap_id, setPCAPID] = useState<number>();
  const [svgName, setSVGName] = useState<string>();

  const submit = () => {
    setPlotting(true);
    const post = { ...config };
    post.sps = post.sps * sps_u;
    setPCAPID(post.pcap_id);
    axios
      .post('http://localhost:5002/dataset/plot', post)
      .then((response) => {
        setSVG(response.data.svg);
        console.log(response.data.df);
      })
      .catch((error) => {
        console.error('There was an error fetching the data!', error);
      })
      .finally(() => {
        setPlotting(false);
      });
  };

  const handleChange = (
    e: React.ChangeEvent<
      HTMLInputElement | HTMLSelectElement | HTMLTextAreaElement
    >,
  ) => {
    const { name, value, type, checked } = e.target;
    if (name === 'range_0') {
      config.range[0] = Number(value);
      setConfig(config);
    } else if (name === 'range_1') {
      config.range[1] = Number(value);
      setConfig(config);
    } else if (type === 'checkbox') {
      setConfig({
        ...config,
        [name]: checked,
      });
    } else if (type === 'number') {
      setConfig({
        ...config,
        [name]: Number(value),
      });
    } else {
      setConfig({
        ...config,
        [name]: value,
      });
    }
  };

  const PCAP = useMemo(
    () => (pcap_id ? () => <PagePcap params={{ id: pcap_id }} /> : () => <></>),
    [pcap_id],
  );

  useEffect(() => {
    const name = JSON.stringify(config).replaceAll('"','').replaceAll(/(\{|\})/g, '').replaceAll(',','__').replaceAll(':','=').replaceAll(/\[(\d+)__(\d+)\]/g, "$1-$2")
    setSVGName(name);
  }, [config]);

  return (
    <main className="flex flex-row min-h-screen flex-col items-center justify-between p-24">
      <div style={{ display: 'flex', flexDirection: 'row' }}>
        <div>
          <label htmlFor="sps">hours:</label>
          <select
            value={sps_u}
            onChange={(e) => {
              const sps_u_new = Number(e.target.value);
              setSPSU(sps_u_new);
              setConfig({ ...config, sps: config.sps * (sps_u / sps_u_new) });
            }}
          >
            <option value={3600}>hours</option>
            <option value={60}>mins</option>
            <option value={1}>secs</option>
          </select>
          <input
            type="text"
            id="sps"
            name="sps"
            value={config.sps}
            onChange={(e) =>
              setConfig({ ...config, sps: Number(e.target.value) })
            }
            required
          />
        </div>
        <div>
          <label htmlFor="onlyfirsts">Only Firsts:</label>
          <select
            id="onlyfirsts"
            name="onlyfirsts"
            value={config.onlyfirsts}
            onChange={handleChange}
            required
          >
            <option value="global">Global</option>
            <option value="pcap">Pcap</option>
            <option value="all">All</option>
          </select>
        </div>

        <div>
          <label htmlFor="packet_type">Packet Type:</label>
          <select
            id="packet_type"
            name="packet_type"
            value={config.packet_type}
            onChange={handleChange}
            required
          >
            <option value="qr">QR</option>
            <option value="q">Q</option>
            <option value="r">R</option>
            <option value="nx">NX</option>
          </select>
        </div>

        <div>
          <label htmlFor="nn">NN:</label>
          <select
            id="nn"
            name="nn"
            value={config.nn}
            onChange={handleChange}
            required
          >
            <option value={NN.NONE}>None</option>
            <option value={NN.TLD}>TLD</option>
            <option value={NN.ICANN}>ICANN</option>
            <option value={NN.PRIVATE}>Private</option>
          </select>
        </div>

        <div>
          <label htmlFor="th">TH:</label>
          <input
            type="number"
            id="th"
            name="th"
            value={config.th}
            onChange={handleChange}
            required
          />
        </div>

        <div>
          <label htmlFor="wl_th">WL TH (Optional):</label>
          <input
            type="number"
            id="wl_th"
            name="wl_th"
            value={config.wl_th || ''}
            onChange={handleChange}
          />
        </div>

        <div>
          <label htmlFor="wl_col">WL Col (Optional):</label>
          <select
            id="wl_col"
            name="wl_col"
            value={config.wl_col || ''}
            onChange={handleChange}
          >
            <option value="">None</option>
            <option value="dn">DN</option>
            <option value="bdn">BDN</option>
          </select>
        </div>

        <div>
          <label htmlFor="pcap_id">Pcap ID (Optional):</label>
          <input
            type="number"
            id="pcap_id"
            name="pcap_id"
            value={config.pcap_id || ''}
            onChange={handleChange}
          />
        </div>
        <div>
          <label htmlFor="pcap_id">Range:</label>
          <input
            type="number"
            id="range_1"
            name="range_1"
            value={config.range[0]}
            onChange={(e) =>
              setConfig({
                ...config,
                range: [Number(e.target.value), config.range[1]],
              })
            }
          />
          <input
            type="number"
            id="range_2"
            name="range_2"
            value={config.range[1]}
            onChange={(e) =>
              setConfig({
                ...config,
                range: [config.range[0], Number(e.target.value)],
              })
            }
          />
        </div>
        <button disabled={plotting} onClick={submit}>
          Set
        </button>
      </div>
      <div style={{ display: 'flex', flexDirection: 'column' }}>
        <div style={{ display: 'flex', flexDirection: 'row' }}>
          <button
            onClick={(e) => {
              // Step 1: Select the SVG element
              var svgElement = document.getElementById('divSVG')?.innerHTML;

              // Step 2: Serialize the SVG XML to a string
              // var serializer = new XMLSerializer();
              // var svgString = serializer.serializeToString(svgElement);

              // Step 3: Create a Blob from the SVG string
              var blob = new Blob([svgElement!], {
                type: 'image/svg+xml;charset=utf-8',
              });

              // Step 4: Create a URL for the Blob
              var url = URL.createObjectURL(blob);

              // Step 5: Create a link element and set the download attribute
              var downloadLink = document.createElement('a');
              downloadLink.href = url;
              downloadLink.download = `${svgName}.svg` ?? 'downloaded.svg';

              // Step 6: Append the link to the body (it won't be visible)
              document.body.appendChild(downloadLink);

              // Step 7: Programmatically click the link to trigger the download
              downloadLink.click();

              // Step 8: Clean up by removing the link and revoking the object URL
              document.body.removeChild(downloadLink);
              URL.revokeObjectURL(url);
            }}
          >
            Download SVG
          </button>
          <input
            type="text"
            style={{ margin: '20px', flexGrow: 1}}
            value={svgName || ""}
            onChange={(e) => setSVGName(e.target.value)}
          />
        </div>
        <div style={{ display: 'flex', flexDirection: 'row' }}>
          <div id="divSVG" dangerouslySetInnerHTML={{ __html: svg }} />
          <div style={{ width: '500px' }}>
            <PCAP />
          </div>
        </div>
      </div>
      {/* <button onClick={(e) => setConfig()}>Load</button> */}
      {/* {svg} */}
    </main>
  );
}
