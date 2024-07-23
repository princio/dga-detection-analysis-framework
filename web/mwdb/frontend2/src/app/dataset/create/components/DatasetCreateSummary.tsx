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

function DatasetsCounter(pcaps: Pcap[]): [string, number][] {
  const datasets = Array.from(new Set(pcaps.map((pcap) => pcap.dataset)));
  const ds_counter = datasets.reduce<{ [key: string]: [string, number] }>(
    (acc, ds) => {
      acc[ds] = [ds, 0];
      return acc;
    },
    {},
  );
  for (const pcap of pcaps) {
    ds_counter[pcap.dataset][1] += 1;
  }
  return Object.values(ds_counter);
}

const columnHelper = createColumnHelper<Pcap>();

const columns = [
  columnHelper.accessor('name', {
    cell: (info) => {
      return info.getValue();
    },
    footer: (info) => info.column.id,
  }),
];

function DatasetCreateSummary() {
  const rerender = React.useReducer(() => ({}), {})[1];

  const { pcaps } = useDatasetCreate();

  const table = useReactTable({
    columns,
    data: pcaps,
    getCoreRowModel: getCoreRowModel(),
  });

  return (
    <div className="p-2">
      <div>
        <div>
          datasets:{' '}
          <table>
            {DatasetsCounter(pcaps).map((item, i) => (
              <tr key={`dataset_create_${i}`}>
                <td>
                  <i>{item[0]}</i>
                </td>
                <td>
                  <b>{item[1]}</b>
                </td>
              </tr>
            ))}
          </table>
        </div>
        <div>qr: {pcaps.reduce((acc, v) => acc + Number(v.qr), 0)}</div>
        <div>q: {pcaps.reduce((acc, v) => acc + Number(v.q), 0)}</div>
        <div>r: {pcaps.reduce((acc, v) => acc + Number(v.r), 0)}</div>
      </div>
      { /*<table>
        <thead>
          {table.getHeaderGroups().map((headerGroup) => (
            <tr key={headerGroup.id}>
              {headerGroup.headers.map((header) => (
                <th key={header.id}>
                  {header.isPlaceholder
                    ? null
                    : flexRender(
                        header.column.columnDef.header,
                        header.getContext(),
                      )}
                </th>
              ))}
            </tr>
          ))}
        </thead>
        <tbody>
          {table.getRowModel().rows.map((row) => (
            <tr key={row.id}>
              {row.getVisibleCells().map((cell) => (
                <td key={cell.id}>
                  {flexRender(cell.column.columnDef.cell, cell.getContext())}
                </td>
              ))}
            </tr>
          ))}
        </tbody>
        <tfoot>
          {table.getFooterGroups().map((footerGroup) => (
            <tr key={footerGroup.id}>
              {footerGroup.headers.map((header) => (
                <th key={header.id}>
                  {header.isPlaceholder
                    ? null
                    : flexRender(
                        header.column.columnDef.footer,
                        header.getContext(),
                      )}
                </th>
              ))}
            </tr>
          ))}
        </tfoot>
      </table>
      <div className="h-4" />
      <button onClick={() => rerender()} className="border p-2">
        Rerender
      </button>
        */}
    </div>
  );
}

export { DatasetCreateSummary };
