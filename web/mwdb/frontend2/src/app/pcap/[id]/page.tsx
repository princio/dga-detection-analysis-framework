'use client';
import { Pcap } from '@/types/Pcap';
import { Message } from '@/types/Message';
import axios from 'axios';
import { useCallback, useEffect, useMemo, useState } from 'react';

function updateTranslation(pcap_id: any, translation: number) {
  return axios.post(`http://localhost:3005/pcap/translate`, {
    pcap_id,
    translation,
  }); // Cambia l'URL con il tuo endpoint NestJS
}

export default function PagePcap({ params }: { params: { id: number } }) {
  const [pcap, setPcap] = useState<Pcap>();
  const [messages, setMessages] = useState<Message[]>();
  const [translation, setTranslation] = useState<string>('');

  useEffect(() => {
    axios
      .get(`http://localhost:3005/pcap/${params.id}`) // Cambia l'URL con il tuo endpoint NestJS
      .then((response) => {
        setPcap(response.data);
        setTranslation(response.data.translation);
      })
      .catch((error) => {
        console.error('There was an error fetching the data!', error);
      });
  }, [params.id]);

  useEffect(() => {
    axios
      .get(`http://localhost:3005/message/pcap/${params.id}?n=10`) // Cambia l'URL con il tuo endpoint NestJS
      .then((response) => {
        console.log(response.data);

        setMessages(response.data);
      })
      .catch((error) => {
        console.error('There was an error fetching the data!', error);
      });
  }, [params.id]);

  if (!pcap) {
    return <div>Pcap not exists.</div>;
  }

  const SummaryTable = () => (
    <table>
      <tbody>
        {Object.entries(pcap)
          .filter(([k, v]) => k !== 'sha256')
          .map(([key, v]) => (
            <tr key={`pcap_id_row_${key}`}>
              <th>{key}</th>
              <td>{key === 'malware' ? v.dga : `${JSON.stringify(v)}`}</td>
            </tr>
          ))}
      </tbody>
    </table>
  );

  return (
    <main className="flex min-h-screen flex-col items-center justify-between p-24">
      <h1>Pcap: {pcap.name}</h1>
      <SummaryTable />

      {messages ? (
        <>
          <table style={{ textAlign: 'center' }}>
            <thead>
              <tr>
                <th>#</th>
                <th>q/r</th>
                <th>rcode</th>
                <th>fnReq</th>
                <th>timeS</th>
                <th>timeSTranslated</th>
              </tr>
            </thead>
            <tbody>
              {messages.map((item: Message, i) => (
                <tr key={`pcap_id_message_${i}`}>
                  <th>{i}</th>
                  <td>{!item.isR ? 'query' : 'response'}</td>
                  <td>
                    {item.isR ? (item.rcode === 3 ? 'NX' : item.rcode) : '-'}
                  </td>
                  <td>{item.fnReq}</td>
                  <td>{item.timeS}</td>
                  <td>{item.timeSTranslated}</td>
                </tr>
              ))}
            </tbody>
          </table>
          <div>
            Set translation (now {pcap.translation} s) to{' '}
            <input
              type="text"
              value={translation !== undefined ? translation : ''}
              onChange={(e) => setTranslation(e.target.value)}
            /> 
            <button
              onClick={(e) => {
                axios
                  .post(`http://localhost:3005/pcap/translate`, {
                    pcap_id: pcap.id,
                    translation,
                  })
                  .then(() => {
                    setPcap((pcap) => (pcap ? {
                      ...pcap,
                      translation: Number(translation)
                    }: undefined));
                  });
              }}
            > 
              Update
            </button>
          </div>
        </>
      ) : (
        <></>
      )}
    </main>
  );
}
