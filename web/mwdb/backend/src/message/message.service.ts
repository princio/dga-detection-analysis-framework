import { Injectable } from '@nestjs/common';
import { InjectRepository } from '@nestjs/typeorm';
import { Message } from 'src/typeorm/entities/Message';
import { Repository } from 'typeorm';

@Injectable()
export class MessageService {
  constructor(
    @InjectRepository(Message)
    private messageRepository: Repository<Message>,
  ) {}

  pcapN(pcap_id: string, n: number = 10): Promise<Message[]> {
    return this.messageRepository.find({
      where: {
        pcapId: pcap_id,
      },
      order: {
        timeS: 'ASC',
        fnReq: 'ASC',
      },
      take: n,
    });
  }

  get(id: string): Promise<Message> {
    return this.messageRepository.findOneBy({ id });
  }
}
