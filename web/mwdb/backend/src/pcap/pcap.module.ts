import { Module } from '@nestjs/common';
import { PcapController } from './pcap.controller';
import { PcapService } from './pcap.service';
import { DataSource } from 'typeorm';
import { Pcap } from 'src/typeorm/entities/Pcap';
import { TypeOrmModule } from '@nestjs/typeorm';

@Module({
  imports: [TypeOrmModule.forFeature([Pcap])],
  controllers: [PcapController],
  providers: [PcapService],
  exports: [PcapService],
})
export class PcapModule {
  constructor(private dataSource: DataSource) {}
}
