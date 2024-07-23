import { Controller, Get, Param } from '@nestjs/common';
import { AppService } from './app.service';
import { Dataset } from './typeorm/entities/Dataset';
import { Pcap } from './typeorm/entities/Pcap';
import { PcapService } from './pcap/pcap.service';
import { DatasetService } from './dataset/dataset.service';

@Controller()
export class AppController {
  constructor(
    private readonly appService: AppService,
    private readonly datasetService: DatasetService,
    private readonly pcapService: PcapService,
  ) {}

  @Get('/datasets')
  getHello(): Promise<Dataset[]> {
    return this.datasetService.findAll();
  }

  @Get('/pcaps')
  getpcaps(): Promise<Pcap[]> {
    return this.pcapService.findAll();
  }

  @Get('/pcap/:id')
  getpcap(@Param('id') id: string): Promise<Pcap> {
    return this.pcapService.get(id);
  }
}
