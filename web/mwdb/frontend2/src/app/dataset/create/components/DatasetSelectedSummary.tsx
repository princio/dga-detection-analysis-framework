'use client';

import axios from 'axios';
import React, { useEffect, useMemo, useState } from 'react';
import {
  RowSelectionState,
  createColumnHelper,
  flexRender,
  getCoreRowModel,
  useReactTable,
} from '@tanstack/react-table';
import { Pcap } from '@/types/Pcap';
import { useDatasetCreate } from '../provider';

const columnHelper = createColumnHelper<Pcap>();

const columns = [
  columnHelper.accessor('name', {
    cell: (info) => {
      return info.getValue();
    },
    footer: (info) => info.column.id,
  }),
];

function DatasetSelectedSummary() {
  const rerender = React.useReducer(() => ({}), {})[1];

  const { pcaps } = useDatasetCreate();
  const [data, setData] = useState<any[3]>();
  const [fetchSummary, setFetchSummary] = useState<boolean>(false);

  useEffect(() => {
    if (fetchSummary) {
      axios
        .get('http://localhost:3005/dataset/summary') // Cambia l'URL con il tuo endpoint NestJS
        .then((response) => {
          setData(response.data);
        })
        .catch((error) => {
          console.error('There was an error fetching the data!', error);
        })
        .finally(() => {
          setFetchSummary(false);
        });
    }
  }, [fetchSummary]);

  const table = useReactTable({
    columns,
    data: pcaps,
    getCoreRowModel: getCoreRowModel(),
  });

  const Button = () => (
    <button
      disabled={fetchSummary}
      onClick={() => {
        setFetchSummary(true);
      }}
    >
      Refresh{fetchSummary ? 'ing' : ''} Summary
    </button>
  );

  if (!data) {
    return (
      <>
        <Button />
      </>
    );
  }

  return (
    <div className="p-2">
      <div
        style={{
          display: 'flex',
          flexDirection: 'row',
          justifyContent: 'center',
        }}
      >
        {Object.keys(data).map((kdga) => (
          <table key={`table_${kdga}`} style={{ margin: '0 10px' }}>
            <thead>
              <tr>
                <th colSpan={2}>{kdga}</th>
              </tr>
            </thead>
            <tbody>
              {Object.entries(data[kdga]).map(([k, v]) => (
                <tr key={`dataset_create_${k}`}>
                  <td>
                    <i>{k}</i>
                  </td>
                  <td>
                    <b>{v as number}</b>
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        ))}
      </div>
      <div>
        <Button />
      </div>
    </div>
  );
}

export { DatasetSelectedSummary };
