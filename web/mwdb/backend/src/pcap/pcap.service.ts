import { Injectable } from '@nestjs/common';
import { InjectRepository } from '@nestjs/typeorm';
import { Pcap } from 'src/typeorm/entities/Pcap';
import { Repository } from 'typeorm';

@Injectable()
export class PcapService {
  constructor(
    @InjectRepository(Pcap)
    private pcapRepository: Repository<Pcap>,
  ) {}

  findAll(): Promise<Pcap[]> {
    return this.pcapRepository.find();
  }

  get(id: string): Promise<Pcap> {
    return this.pcapRepository.findOneBy({ id });
  }

  select(id: string, selected: boolean): Promise<any> {
    return this.pcapRepository.update(id, { selected });
  }

  translate(id: string, translation: number): Promise<any> {
    console.log('translation');
    return this.pcapRepository.update(id, { translation });
  }
}
