import { Column, Entity, Index, JoinColumn, ManyToOne } from "typeorm";
import { Whitelist } from "./Whitelist";

@Index("whitelist_list_1_pkey", ["id", "whitelistId"], { unique: true })
@Entity("whitelist_list_1", { schema: "public" })
export class WhitelistList_1 {
  @Column("bigint", { primary: true, name: "id" })
  id: string;

  @Column("bigint", { primary: true, name: "whitelist_id" })
  whitelistId: string;

  @Column("text", { name: "dn" })
  dn: string;

  @Column("integer", { name: "rank", nullable: true })
  rank: number | null;

  @ManyToOne(() => Whitelist, (whitelist) => whitelist.whitelistListS)
  @JoinColumn([{ name: "whitelist_id", referencedColumnName: "id" }])
  whitelist: Whitelist;
}
