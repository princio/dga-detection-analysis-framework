'use client'

import axios from 'axios';
import React, { useEffect, useState } from 'react';
import {
  createColumnHelper,
  flexRender,
  getCoreRowModel,
  useReactTable,
} from '@tanstack/react-table'
import { Dataset } from '@/types/Dataset';



const columnHelper = createColumnHelper<Dataset>()

const columns = [
    columnHelper.accessor('name', {
      cell: info => {
        return info.getValue();
      },
      footer: info => info.column.id,
    }),
    columnHelper.accessor('pcaps', {
      cell: info => {
        return info.getValue().length;
      },
      footer: info => info.column.id,
    }),
    ...[0,1,2].map((dga) => 
    columnHelper.accessor('pcaps', {
        id: `dga=${dga}`,
      cell: info => {
        return info.getValue().filter((pcap) => pcap.malware.dga === dga).length; //.map((pcap) => pcap.name);
      },
      footer: info => info.column.id,
    })),
    ...[0,1,2].map((dga) => 
    columnHelper.accessor('pcaps', {
        id: `u=${dga}`,
      cell: info => {
        return info.getValue().filter((pcap) => pcap.u).reduce((acc, pcap) => (acc + pcap.u), 0); //.map((pcap) => pcap.name);
      },
      footer: info => info.column.id,
    })),
];

function DatasetTable() {
    const [data, setData] = useState([]);
    const rerender = React.useReducer(() => ({}), {})[1]

    useEffect(() => {
      axios.get('http://localhost:3005/datasets') // Cambia l'URL con il tuo endpoint NestJS
        .then(response => {
          setData(response.data);
        })
        .catch(error => {
          console.error('There was an error fetching the data!', error);
        });
    }, []);

    const table = useReactTable({
        columns,
        data,
        getCoreRowModel: getCoreRowModel(),
    });

        return (
          <div className="p-2">
            <table>
              <thead>
                {table.getHeaderGroups().map(headerGroup => (
                  <tr key={headerGroup.id}>
                    {headerGroup.headers.map(header => (
                      <th key={header.id}>
                        {header.isPlaceholder
                          ? null
                          : flexRender(
                              header.column.columnDef.header,
                              header.getContext()
                            )}
                      </th>
                    ))}
                  </tr>
                ))}
              </thead>
              <tbody>
                {table.getRowModel().rows.map(row => (
                  <tr key={row.id}>
                    {row.getVisibleCells().map(cell => (
                      <td key={cell.id}>
                        {flexRender(cell.column.columnDef.cell, cell.getContext())}
                      </td>
                    ))}
                  </tr>
                ))}
              </tbody>
              <tfoot>
                {table.getFooterGroups().map(footerGroup => (
                  <tr key={footerGroup.id}>
                    {footerGroup.headers.map(header => (
                      <th key={header.id}>
                        {header.isPlaceholder
                          ? null
                          : flexRender(
                              header.column.columnDef.footer,
                              header.getContext()
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
};

export { DatasetTable };