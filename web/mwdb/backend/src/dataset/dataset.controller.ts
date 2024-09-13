import { Controller, Get } from '@nestjs/common';
import { DatasetService } from './dataset.service';
import { CacheKey, CacheTTL } from '@nestjs/cache-manager';

@Controller('dataset')
// @UseInterceptors(CacheInterceptor)
export class DatasetController {
  constructor(private readonly service: DatasetService) {}

  @Get('refresh')
  refresh() {
    return this.service.refresh();
  }

  @CacheKey('dataset_summary')
  @CacheTTL(24 * 3600)
  @Get('summary')
  summary() {
    return this.service.selectSummary();
  }
}
