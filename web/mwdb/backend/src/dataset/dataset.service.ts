import { Injectable } from '@nestjs/common';
import { InjectRepository } from '@nestjs/typeorm';
import { Dataset } from 'src/typeorm/entities/Dataset';
import { Repository } from 'typeorm';

@Injectable()
export class DatasetService {
  constructor(
    @InjectRepository(Dataset)
    private repo: Repository<Dataset>,
  ) {}

  findAll(): Promise<Dataset[]> {
    return this.repo.find({
      loadEagerRelations: true,
    });
  }
}
