import { Controller, Get, Param, Query } from '@nestjs/common';
import { MessageService } from './message.service';

@Controller('message')
export class MessageController {
  constructor(private readonly service: MessageService) {}

  @Get('pcap/:pcap_id')
  pcapN(@Param('pcap_id') pcap_id: string, @Query('n') n: number) {
    return this.service.pcapN(pcap_id, n);
  }
}
