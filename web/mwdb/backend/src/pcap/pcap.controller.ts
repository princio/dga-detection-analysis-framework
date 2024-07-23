import { Body, Controller, Post } from '@nestjs/common';
import { PcapService } from './pcap.service';

@Controller('pcap')
export class PcapController {
  constructor(private readonly service: PcapService) {}

  @Post('select')
  select(@Body() { id, selected }: { id: number; selected: boolean }) {
    return this.service.select(String(id), selected);
  }

  @Post('translate')
  translate(
    @Body() { pcap_id, translation }: { pcap_id: number; translation: number },
  ) {
    return this.service.translate(String(pcap_id), translation);
  }
}
