import {
  Column,
  Entity,
  Index,
  JoinColumn,
  ManyToMany,
  ManyToOne,
  PrimaryGeneratedColumn,
} from 'typeorm';
import { Malware } from './Malware';
import { Dataset } from './Dataset';

@Index('pcap_pkey', ['id'], { unique: true })
@Index('pcap_sha256_unique', ['sha256'], { unique: true })
@Entity('pcap', { schema: 'public' })
export class Pcap {
  @PrimaryGeneratedColumn({ type: 'bigint', name: 'id' })
  id: string;

  @Column('character varying', { name: 'name', length: 255 })
  name: string;

  @Column('character', { name: 'sha256', unique: true, length: 64 })
  sha256: string;

  @Column('boolean', { name: 'infected' })
  infected: boolean;

  @Column('text', { name: 'dataset' })
  dataset: string;

  @Column('integer', { name: 'day', nullable: true })
  day: number | null;

  @Column('integer', { name: 'days', nullable: true })
  days: number | null;

  @Column('text', { name: 'terminal', nullable: true })
  terminal: string | null;

  @Column('bigint', { name: 'qr' })
  qr: string;

  @Column('bigint', { name: 'q' })
  q: string;

  @Column('bigint', { name: 'r' })
  r: string;

  @Column('bigint', { name: 'u' })
  u: string;

  @Column('bigint', { name: 'fnreq_max' })
  fnreqMax: string;

  @Column('real', { name: 'dga_ratio', precision: 24 })
  dgaRatio: number;

  @Column('json', { name: 'batches', nullable: true })
  batches: object | null;

  @Column('integer', { name: 'year', nullable: true })
  year: number | null;

  @Column('real', { name: 'duration', nullable: true, precision: 24 })
  duration: number | null;

  @Column('character varying', { name: 'mcfp_id', nullable: true, length: 100 })
  mcfpId: string | null;

  @Column('enum', {
    name: 'mcfp_type',
    nullable: true,
    enum: ['BOTNET', 'MIXED', 'NORMAL', 'OTHER'],
  })
  mcfpType: 'BOTNET' | 'MIXED' | 'NORMAL' | 'OTHER' | null;

  @Column('json', { name: 'mcfp_info', nullable: true })
  mcfpInfo: object | null;

  @ManyToMany(() => Dataset, (dataset) => dataset.pcaps)
  datasets: Dataset[];

  @ManyToOne(() => Malware, (malware) => malware.pcaps)
  @JoinColumn([{ name: 'malware_id', referencedColumnName: 'id' }])
  malware: Malware;
}
