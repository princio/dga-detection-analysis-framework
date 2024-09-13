import { Module } from '@nestjs/common';
import { AppController } from './app.controller';
import { AppService } from './app.service';
import { PcapModule } from './pcap/pcap.module';
import { TypeOrmModule } from '@nestjs/typeorm';
import { Pcap } from './typeorm/entities/Pcap';
import { Malware } from './typeorm/entities/Malware';
import { Dataset } from './typeorm/entities/Dataset';
import { DatasetModule } from './dataset/dataset.module';
import { MessageModule } from './message/message.module';
import { Message } from './typeorm/entities/Message';
import { CacheModule } from '@nestjs/cache-manager';

@Module({
  imports: [
    CacheModule.register({
      isGlobal: true,
    }),
    TypeOrmModule.forRoot({
      type: 'postgres',
      host: 'localhost',
      port: 5432,
      username: 'postgres',
      password: 'postgres',
      database: 'dns_mac',
      entities: [Pcap, Malware, Dataset, Message],
      synchronize: false,
    }),
    PcapModule,
    DatasetModule,
    MessageModule,
  ],
  controllers: [AppController],
  providers: [AppService],
})
export class AppModule {}
