'use client';

import axios from 'axios';
import React, { HTMLProps, useEffect, useState } from 'react';
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
import { Dataset } from '@/types/Dataset';
import { Pcap } from '@/types/Pcap';
import { DatasetTable2 } from './dataset/components/DatasetSelectedSummary';

function PcapTable2() {
  const rerender = React.useReducer(() => ({}), {})[1];

  const [rowSelection, setRowSelection] = React.useState({});
  const [globalFilter, setGlobalFilter] = React.useState('');
  const [sorting, setSorting] = React.useState<SortingState>([]);

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
        header: 'Name',
        footer: (props) => props.column.id,
        columns: [
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
        ],
      },
      {
        header: 'Info',
        footer: (props) => props.column.id,
        columns: [
          {
            accessorKey: 'malware.dga',
            header: () => 'dga',
            footer: (props) => props.column.id,
          },
          {
            header: 'More Info',
            columns: [
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
            ],
          },
        ],
      },
    ],
    [],
  );

  const [data, setData] = React.useState<Pcap[]>([]);
  const [dataSelected, setDataSelected] = React.useState<RowSelectionState>([]);

  const [refreshKey, setRefreshKey] = useState(0);

  useEffect(() => {
    axios
      .get('http://localhost:3005/pcaps') // Cambia l'URL con il tuo endpoint NestJS
      .then((response) => {
        setData(response.data);
      })
      .catch((error) => {
        console.error('There was an error fetching the data!', error);
      });
  }, [refreshKey]);

  function refreshData() {
    setRefreshKey((old) => old + 1);
  }

  const table = useReactTable({
    data,
    columns,
    state: {
      rowSelection,
      sorting,
    },
    getSortedRowModel: getSortedRowModel(),
    onSortingChange: setSorting,
    enableRowSelection: true,
    // enableRowSelection: row => row.original.age > 18, // or enable row selection conditionally per row
    onRowSelectionChange: (update) => {
      setRowSelection(update);
      console.log(typeof update);

      setDataSelected(update);
    },
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

  return typeof firstValue === 'number' ? (
    <div className="flex space-x-2">
      <input
        type="number"
        value={((column.getFilterValue() as any)?.[0] ?? '') as string}
        onChange={(e) =>
          column.setFilterValue((old: any) => [e.target.value, old?.[1]])
        }
        placeholder={`Min`}
        className="w-24 border shadow rounded"
      />
      <input
        type="number"
        value={((column.getFilterValue() as any)?.[1] ?? '') as string}
        onChange={(e) =>
          column.setFilterValue((old: any) => [old?.[0], e.target.value])
        }
        placeholder={`Max`}
        className="w-24 border shadow rounded"
      />
    </div>
  ) : (
    <input
      type="text"
      value={(column.getFilterValue() ?? '') as string}
      onChange={(e) => column.setFilterValue(e.target.value)}
      placeholder={`Search...`}
      className="w-36 border shadow rounded"
    />
  );
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

export { PcapTable2 };
