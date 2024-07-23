'use client';

import axios from 'axios';
import React, { useEffect, useState } from 'react';
import {
  createColumnHelper,
  flexRender,
  getCoreRowModel,
  useReactTable,
} from '@tanstack/react-table';
import { Pcap } from '@/types/Pcap';

const columnHelper = createColumnHelper<Pcap>();

const columns = [
  columnHelper.accessor('name', {
    cell: (info) => {
      return info.getValue();
    },
    footer: (info) => info.column.id,
  }),
  columnHelper.accessor('malware.dga', {
    cell: (info) => {
      return info.getValue();
    },
    footer: (info) => info.column.id,
  }),
  columnHelper.accessor('dataset', {
    cell: (info) => {
      return info.getValue();
    },
    footer: (info) => info.column.id,
  }),
  columnHelper.accessor('qr', {
    cell: (info) => {
      return info.getValue();
    },
    footer: (info) => info.column.id,
  }),
  columnHelper.accessor('q', {
    cell: (info) => {
      return info.getValue();
    },
    footer: (info) => info.column.id,
  }),
  columnHelper.accessor('r', {
    cell: (info) => {
      return info.getValue();
    },
    footer: (info) => info.column.id,
  }),
  columnHelper.accessor('u', {
    cell: (info) => {
      return info.getValue();
    },
    footer: (info) => info.column.id,
  }),
];

function PcapTable() {
  const [data, setData] = useState([]);
  const rerender = React.useReducer(() => ({}), {})[1];

  useEffect(() => {
    axios
      .get('http://localhost:3005/pcaps') // Cambia l'URL con il tuo endpoint NestJS
      .then((response) => {
        setData(response.data);
      })
      .catch((error) => {
        console.error('There was an error fetching the data!', error);
      });
  }, []);

  const table = useReactTable({
    columns,
    data,
    getCoreRowModel: getCoreRowModel(),
    enableRowSelection: true,
  });

  return (
    <div className="p-2">
      <table>
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
    </div>
  );
}

export { PcapTable };
