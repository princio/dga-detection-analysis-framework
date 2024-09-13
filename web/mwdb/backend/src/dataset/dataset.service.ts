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

  async refresh(): Promise<void> {
    console.log('refresh start');
    for (const packet of ['nx', 'r', 'q', 'qr']) {
      for (const grouping of ['global', 'pcap']) {
        console.log(`${packet}_${grouping} doing...`);
        await this.repo.query(
          `REFRESH MATERIALIZED VIEW public.${packet}_${grouping} WITH DATA;`,
        );
        console.log(`${packet}_${grouping} done.`);
      }
    }
    console.log('refresh end');
  }

  async selectSummary(): Promise<any> {
    const ret = {};
    for (const dga of ['all', 'no-malware', 'non-dga', 'dga']) {
      const withs = [];
      const froms = [];
      let where = '';
      if (dga !== 'all') {
        where = ` WHERE pcap_malware_type='${dga}'`;
      }
      for (const packet of ['qr', 'q', 'r']) {
        for (const grouping of ['all', 'global', 'pcap']) {
          withs.push(
            `${packet}_${grouping} as (SELECT COUNT(*) as ${packet}_${grouping} FROM ${packet}_${grouping} ${where})`,
          );
          froms.push(`${packet}_${grouping}`);
        }
      }
      ret[dga] = (
        await this.repo.query(`
    WITH
      ${withs.join(',')}
      SELECT * FROM ${froms.join(',')}
    `)
      )[0];
    }

    return ret;
  }
}
