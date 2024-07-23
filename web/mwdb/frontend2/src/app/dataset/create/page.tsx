'use client';

import { DatasetSelectedSummary } from './components/DatasetSelectedSummary';
import { DatasetCreateProvider, useDatasetCreate } from './provider';
import PcapSelectTable from './components/PcapSelectTable';
import { useEffect, useState } from 'react';
import axios from 'axios';

enum Refresh {
  idle = 0,
  started = 2,
  error = 3,
}

export default function Home() {
  const { setName, pcaps } = useDatasetCreate();
  const [refresh_status, setRefreshStatus] = useState<Refresh>(Refresh.idle);
  const [refresh_start, setRefreshStart] = useState<boolean>(false);

  useEffect(() => {
    console.log(refresh_start);
    
    if (refresh_start) {
      setRefreshStatus(Refresh.started);
      axios
        .get('http://localhost:3005/dataset/refresh')
        .then((response) => {
          setRefreshStatus(Refresh.idle);
        })
        .catch((error) => {
          console.error('There was an error fetching the data!', error);
          setRefreshStatus(Refresh.error);
        });
    }
    setRefreshStart(false);
  }, [refresh_start]);

  return (
    <main className="flex min-h-screen flex-col items-center justify-between p-24">
      <DatasetCreateProvider>
        <h1>
          Create dataset:{' '}
          <input type="text" onChange={(e) => setName(e.target.value)} />
        </h1>
        <div>
          <DatasetSelectedSummary />
          <PcapSelectTable />
        </div>
      </DatasetCreateProvider>
      <button
        onClick={() => setRefreshStart(true)}
        style={{color: refresh_status === Refresh.error ? 'red' : 'black'}}
        disabled={refresh_status !== Refresh.idle}
      >
        Refresh{refresh_status === Refresh.started ? 'ing' : ''}
      </button>
    </main>
  );
}
