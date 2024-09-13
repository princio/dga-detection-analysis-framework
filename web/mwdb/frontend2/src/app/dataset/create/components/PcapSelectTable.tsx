'use client';

import axios from 'axios';
import React, { HTMLProps, useCallback, useEffect, useMemo, useState } from 'react';
import {
  Column,
  ColumnDef,
  RowSelectionState,
  SortingState,
  Table,
  createColumnHelper,
  flexRender,
  getCoreRowModel,
  getFilteredRowModel,
  getPaginationRowModel,
  getSortedRowModel,
  useReactTable,
} from '@tanstack/react-table';
import { Pcap } from '@/types/Pcap';
import { useDatasetCreate } from '../provider';



export default function PcapSelectTable() {
  const rerender = React.useReducer(() => ({}), {})[1];

  const [rowSelection, setRowSelection] = React.useState({});
  const [globalFilter, setGlobalFilter] = React.useState('');
  const [sorting, setSorting] = React.useState<SortingState>([]);
  const { PCAPS, setPcaps, setPCAPS } = useDatasetCreate();

  useEffect(() => {
    setPcaps(Object.keys(rowSelection).map((idx) => PCAPS[Number(idx)]));
  }, [setPcaps, rowSelection, PCAPS]);

  const checkSelected = useCallback((index: number, checked: boolean) => {
    console.log(index, checked, PCAPS[index].name);
    
    axios
    .post('http://localhost:3005/pcap/select', { id: PCAPS[index].id, selected: checked })
    .then((response) => {
      console.log(response);
      PCAPS[index].selected = true;
      setPCAPS(PCAPS.map((pcap) => (pcap.selected = pcap.id===PCAPS[index].id ? checked : pcap.selected, pcap)));
    })
    .catch((error) => {
      console.error('There was an error fetching the data!', error);
    });
  }, [PCAPS, setPCAPS]);

  const columns = React.useMemo<ColumnDef<Pcap>[]>(
    () => [
      {
        id: 'select',
        header: ({ table }) => (
          <IndeterminateCheckbox
            {...{
              checked: table.getIsAllRowsSelected(),
              indeterminate: table.getIsSomeRowsSelected(),
              onChange: table.getToggleAllRowsSelectedHandler(),
            }}
          />
        ),
        cell: ({ row }) => (
          <div className="px-1">
            <IndeterminateCheckbox
              {...{
                checked: row.getIsSelected(),
                disabled: !row.getCanSelect(),
                indeterminate: row.getIsSomeSelected(),
                onChange: row.getToggleSelectedHandler(),
              }}
            />
          </div>
        ),
      },
      {
        accessorKey: 'id',
        cell: (info) => info.getValue(),
        footer: (props) => props.column.id,
      },
      {
        accessorKey: 'name',
        cell: (info) => info.getValue(),
        footer: (props) => props.column.id,

      },
      {
        accessorFn: (row) => row.dataset,
        id: 'dataset',
        cell: (info) => info.getValue(),
        footer: (props) => props.column.id,
      },
      {
        accessorKey: 'malware.dga',
        header: () => 'dga',
        footer: (props) => props.column.id,
      },
      {
        accessorKey: 'year',
        header: () => 'year',
        footer: (props) => props.column.id,
      },
      {
        header: () => 'duration',
        footer: (props) => props.column.id,
        accessorFn: (row) => (row.duration ? row.duration : '-'),
        id: 'duration'
      },
      {
        header: () => 'translation',
        footer: (props) => props.column.id,
        accessorFn: (row) => (row.translation ? row.translation : '-'),
        id: 'translation'
      },
      {
        header: () => 'first_message_time_s',
        footer: (props) => props.column.id,
        accessorFn: (row) => (row.first_message_time_s ? row.first_message_time_s : '-'),
        id: 'first_message_time_s'
      },
      {
        header: 'u',
        accessorFn: (row) => Number(row.u),
        footer: (props) => props.column.id,
      },
      {
        header: 'qr',
        accessorFn: (row) => Number(row.qr),
        footer: (props) => props.column.id,
      },
      {
        header: 'q',
        accessorFn: (row) => Number(row.q),
        footer: (props) => props.column.id,
      },
      {
        header: 'r',
        accessorFn: (row) => Number(row.r),
        footer: (props) => props.column.id,
      },
      {
        header: 'selected',
        accessorKey: 'selected',
        footer: (props) => props.column.id,
        cell: (info) => (
          <input
            type="checkbox"
            className={' cursor-pointer'}
            value={info.getValue() ? 'true' : 'false'}
            onChange={((e) => checkSelected(info.row.index, e.target.checked))}
            checked={info.getValue() as boolean}
          />
        ),
      },
    ],
    [checkSelected],
  );

  const table = useReactTable({
    data: PCAPS,
    columns,
    state: {
      rowSelection,
      sorting,
    },
    getSortedRowModel: getSortedRowModel(),
    onSortingChange: setSorting,
    enableRowSelection: true,
    // enableRowSelection: row => row.original.age > 18, // or enable row selection conditionally per row
    onRowSelectionChange: setRowSelection,
    getCoreRowModel: getCoreRowModel(),
    getFilteredRowModel: getFilteredRowModel(),
    getPaginationRowModel: getPaginationRowModel(),
    debugTable: false,
  });

  return (
    <div className="p-2" style={{ display: 'flex', flexDirection: 'row' }}>
      <div>
        <div>
          <input
            value={globalFilter ?? ''}
            onChange={(e) => setGlobalFilter(e.target.value)}
            className="p-2 font-lg shadow border border-block"
            placeholder="Search all columns..."
          />
        </div>
        <div className="h-2" />
        <table>
          <thead>
            {table.getHeaderGroups().map((headerGroup) => (
              <tr key={headerGroup.id}>
                {headerGroup.headers.map((header) => {
                  return (
                    <th key={header.id} colSpan={header.colSpan}>
                      {header.isPlaceholder ? null : (
                        <div
                          className={
                            header.column.getCanSort()
                              ? 'cursor-pointer select-none'
                              : ''
                          }
                          onClick={header.column.getToggleSortingHandler()}
                          title={
                            header.column.getCanSort()
                              ? header.column.getNextSortingOrder() === 'asc'
                                ? 'Sort ascending'
                                : header.column.getNextSortingOrder() === 'desc'
                                ? 'Sort descending'
                                : 'Clear sort'
                              : undefined
                          }
                        >
                          {flexRender(
                            header.column.columnDef.header,
                            header.getContext(),
                          )}
                          {header.column.getCanFilter() ? (
                            <div>
                              <Filter column={header.column} table={table} />
                            </div>
                          ) : null}
                          {{
                            asc: ' 🔼',
                            desc: ' 🔽',
                          }[header.column.getIsSorted() as string] ?? null}
                        </div>
                      )}
                    </th>
                  );
                })}
              </tr>
            ))}
          </thead>
          <tbody>
            {table.getRowModel().rows.map((row) => {
              return (
                <tr key={row.id}>
                  {row.getVisibleCells().map((cell) => {
                    return (
                      <td key={cell.id}>
                        {flexRender(
                          cell.column.columnDef.cell,
                          cell.getContext(),
                        )}
                      </td>
                    );
                  })}
                </tr>
              );
            })}
          </tbody>
          <tfoot>
            <tr>
              <td className="p-1">
                <IndeterminateCheckbox
                  {...{
                    checked: table.getIsAllPageRowsSelected(),
                    indeterminate: table.getIsSomePageRowsSelected(),
                    onChange: table.getToggleAllPageRowsSelectedHandler(),
                  }}
                />
              </td>
              <td colSpan={20}>
                Page Rows ({table.getRowModel().rows.length})
              </td>
            </tr>
          </tfoot>
        </table>
        <div className="h-2" />
        <div className="flex items-center gap-2">
          <button
            className="border rounded p-1"
            onClick={() => table.setPageIndex(0)}
            disabled={!table.getCanPreviousPage()}
          >
            {'<<'}
          </button>
          <button
            className="border rounded p-1"
            onClick={() => table.previousPage()}
            disabled={!table.getCanPreviousPage()}
          >
            {'<'}
          </button>
          <button
            className="border rounded p-1"
            onClick={() => table.nextPage()}
            disabled={!table.getCanNextPage()}
          >
            {'>'}
          </button>
          <button
            className="border rounded p-1"
            onClick={() => table.setPageIndex(table.getPageCount() - 1)}
            disabled={!table.getCanNextPage()}
          >
            {'>>'}
          </button>
          <span className="flex items-center gap-1">
            <div>Page</div>
            <strong>
              {table.getState().pagination.pageIndex + 1} of{' '}
              {table.getPageCount()}
            </strong>
          </span>
          <span className="flex items-center gap-1">
            | Go to page:
            <input
              type="number"
              defaultValue={table.getState().pagination.pageIndex + 1}
              onChange={(e) => {
                const page = e.target.value ? Number(e.target.value) - 1 : 0;
                table.setPageIndex(page);
              }}
              className="border p-1 rounded w-16"
            />
          </span>
          <select
            value={table.getState().pagination.pageSize}
            onChange={(e) => {
              table.setPageSize(Number(e.target.value));
            }}
          >
            {[10, 20, 30, 40, 50].map((pageSize) => (
              <option key={pageSize} value={pageSize}>
                Show {pageSize}
              </option>
            ))}
          </select>
        </div>
        <br />
        <div>
          {Object.keys(rowSelection).length} of{' '}
          {table.getPreFilteredRowModel().rows.length} Total Rows Selected
        </div>
        <hr />
        {/* <br />
      <div>
        <button className="border rounded p-2 mb-2" onClick={() => rerender()}>
          Force Rerender
        </button>
      </div>
      <div>
        <button
          className="border rounded p-2 mb-2"
          onClick={() => refreshData()}
        >
          Refresh Data
        </button>
      </div> */}
        {/* <div>
        <button
          className="border rounded p-2 mb-2"
          onClick={() =>
            console.info(
              'table.getSelectedRowModel().flatRows',
              table.getSelectedRowModel().flatRows
            )
          }
        >
          Log table.getSelectedRowModel().flatRows
        </button>
      </div> */}
        {/* <div>
        <label>Row Selection State:</label>
        <pre>{JSON.stringify(table.getState().rowSelection, null, 2)}</pre>
        <pre>{JSON.stringify(Object.keys(table.getState().rowSelection).map((key) => data[Number(key)].id), null, 2)}</pre>
      </div> */}
      </div>
    </div>
  );
}

function Filter({
  column,
  table,
}: {
  column: Column<any, any>;
  table: Table<any>;
}) {
  const firstValue = table
    .getPreFilteredRowModel()
    .flatRows[0]?.getValue(column.id);
    
  if (typeof firstValue === 'number') {
    return (
      <div className="flex space-x-2">
        <input
          type="number"
          value={((column.getFilterValue() as any)?.[0] ?? '') as string}
          onChange={(e) =>
            column.setFilterValue((old: any) => [e.target.value, old?.[1]])
          }
          placeholder={`m`}
          className="w-8 border shadow rounded"
        />
        <input
          type="number"
          value={((column.getFilterValue() as any)?.[1] ?? '') as string}
          onChange={(e) =>
            column.setFilterValue((old: any) => [old?.[0], e.target.value])
          }
          placeholder={`M`}
          className="w-8 border shadow rounded"
        />
      </div>
    );
  } else if (typeof firstValue === 'string') {
    return (
      <input
        type="text"
        value={(column.getFilterValue() ?? '') as string}
        onChange={(e) => column.setFilterValue(e.target.value)}
        placeholder={`Search...`}
        className="w-36 border shadow rounded"
      />
    );
  } else if (typeof firstValue === 'boolean') {
    return (
      <input
        type="checkbox"
        checked={(column.getFilterValue()) as boolean}
        onChange={(e) => column.setFilterValue((updater: boolean) => {
          return e.target.checked;
        })}
        className="w-36 border shadow rounded"
      />
    );
  }
}

function IndeterminateCheckbox({
  indeterminate,
  className = '',
  ...rest
}: { indeterminate?: boolean } & HTMLProps<HTMLInputElement>) {
  const ref = React.useRef<HTMLInputElement>(null!);

  React.useEffect(() => {
    if (typeof indeterminate === 'boolean') {
      ref.current.indeterminate = !rest.checked && indeterminate;
    }
  }, [ref, indeterminate]);

  return (
    <input
      type="checkbox"
      ref={ref}
      className={className + ' cursor-pointer'}
      {...rest}
    />
  );
}
