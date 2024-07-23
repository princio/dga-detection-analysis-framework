import { Column, Entity, PrimaryGeneratedColumn } from 'typeorm';

@Entity('message', { schema: 'public' })
export class Message {
  @PrimaryGeneratedColumn({ type: 'bigint', name: 'id' })
  id: string;

  @Column('bigint', { name: 'pcap_id' })
  pcapId: string;

  @Column('real', { name: 'time_s', precision: 24 })
  timeS: number;

  @Column('bigint', { name: 'fn' })
  fn: string;

  @Column('bigint', { name: 'fn_req' })
  fnReq: string;

  @Column('bigint', { name: 'dn_id' })
  dnId: string;

  @Column('integer', { name: 'qcode' })
  qcode: number;

  @Column('boolean', { name: 'is_r' })
  isR: boolean;

  @Column('integer', { name: 'rcode', nullable: true })
  rcode: number | null;

  @Column('character varying', { name: 'server', nullable: true, length: 20 })
  server: string | null;

  @Column('text', { name: 'answer', nullable: true })
  answer: string | null;

  @Column('real', { name: 'time_s_translated', nullable: true, precision: 24 })
  timeSTranslated: number | null;

  @Column('real', { name: 'tmp_min_time', nullable: true, precision: 24 })
  tmpMinTime: number | null;

  @Column('boolean', { name: 'pcap_infected', nullable: true })
  pcapInfected: boolean | null;

  @Column('enum', {
    name: 'pcap_malware_type',
    nullable: true,
    enum: ['non-dga', 'dga', 'no-malware'],
  })
  pcapMalwareType: 'non-dga' | 'dga' | 'no-malware' | null;
}
