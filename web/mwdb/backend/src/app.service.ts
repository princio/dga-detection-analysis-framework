import { Injectable } from '@nestjs/common';
import { PcapService } from './pcap/pcap.service';
import { DatasetService } from './dataset/dataset.service';
import { Dataset } from './typeorm/entities/Dataset';

@Injectable()
export class AppService {
  constructor(
    private pcapService: PcapService,
    private datasetService: DatasetService,
  ) {}
  async getHello(): Promise<Dataset[]> {
    const data = await this.datasetService.findAll();
    return data;
    // .filter((item) => item.pcaps)
    // .flatMap((item) => item.pcaps.map((pcap) => pcap.name));
  }
}
