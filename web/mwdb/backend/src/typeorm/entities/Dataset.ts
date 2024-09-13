import {
  Column,
  Entity,
  Index,
  JoinTable,
  ManyToMany,
  PrimaryGeneratedColumn,
} from 'typeorm';
import { Pcap } from './Pcap';

@Index('dataset_pkey', ['id'], { unique: true })
@Entity('dataset', { schema: 'public' })
export class Dataset {
  @PrimaryGeneratedColumn({ type: 'bigint', name: 'id' })
  id: string;

  @Column('text', { name: 'name' })
  name: string;

  @ManyToMany(() => Pcap, (pcap) => pcap.datasets, { eager: true })
  @JoinTable({
    name: 'dataset_pcap',
    joinColumn: {
      name: 'dataset_id',
      referencedColumnName: 'id',
    },
    inverseJoinColumn: {
      name: 'pcap_id',
      referencedColumnName: 'id',
    },
  })
  pcaps: Pcap[];
}
