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
}
