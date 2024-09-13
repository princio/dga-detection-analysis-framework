'use client'

import { Pcap } from '@/types/Pcap';
import axios from 'axios';
import React, {
  createContext,
  useState,
  useContext,
  ReactNode,
  useEffect,
} from 'react';

interface DatasetCreateContextType {
  name?: string;
  setName: (name: string) => void,
  pcaps: Pcap[];
  setPcaps: (pcaps: Pcap[]) => void,
  PCAPS: Pcap[];
  setPCAPS: (pcaps: Pcap[]) => void,
}

const DatasetCreateContext = createContext<
  DatasetCreateContextType
>({
  name: undefined,
  setName: (name) => {},
  pcaps: [],
  setPcaps: (pcaps) => {},
  PCAPS: [],
  setPCAPS: (pcaps) => {},
});

const DatasetCreateProvider: React.FC<{ children: ReactNode }> = ({
  children,
}) => {
  const [name, setName] = useState<string>();
  const [PCAPS, setPCAPS] = useState<Pcap[]>([]);
  const [pcaps, setPcaps] = useState<Pcap[]>([]);

  useEffect(() => {
    axios
      .get('http://localhost:3005/pcaps') // Cambia l'URL con il tuo endpoint NestJS
      .then((response) => {
        setPCAPS(response.data);
      })
      .catch((error) => {
        console.error('There was an error fetching the data!', error);
      });
  }, []);

  return (
    <DatasetCreateContext.Provider value={{ name, setName, pcaps, setPcaps, PCAPS, setPCAPS }}>
      {children}
    </DatasetCreateContext.Provider>
  );
};

const useDatasetCreate = (): DatasetCreateContextType => {
  const context = useContext(DatasetCreateContext);
  if (!context) {
    throw new Error(
      'useDatasetCreate must be used within a DatasetCreateProvider',
    );
  }
  return context;
};

export { DatasetCreateProvider, useDatasetCreate };
