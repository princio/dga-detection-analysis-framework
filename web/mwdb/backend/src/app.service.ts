import { Injectable } from '@nestjs/common';
import { PcapService } from './pcap/pcap.service';
import { DatasetService } from './dataset/dataset.service';

@Injectable()
export class AppService {
  constructor(
    private pcapService: PcapService,
    private datasetService: DatasetService,
  ) {}
  async getHello(): Promise<string[]> {
    const data = await this.datasetService.findAll();
    return data
      .filter((item) => item.pcaps)
      .flatMap((item) => item.pcaps.map((pcap) => pcap.name));
  }
}
