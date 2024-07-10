import { Column, Entity } from "typeorm";

@Entity("ids", { schema: "public" })
export class Ids {
  @Column("int8", { name: "array_agg", nullable: true, array: true })
  arrayAgg: string[] | null;
}
