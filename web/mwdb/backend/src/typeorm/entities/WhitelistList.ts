import {
  Column,
  Entity,
  JoinColumn,
  ManyToOne,
  PrimaryGeneratedColumn,
} from "typeorm";
import { Whitelist } from "./Whitelist";

@Entity("whitelist_list", { schema: "public" })
export class WhitelistList {
  @PrimaryGeneratedColumn({ type: "bigint", name: "id" })
  id: string;

  @Column("text", { name: "dn" })
  dn: string;

  @Column("integer", { name: "rank", nullable: true })
  rank: number | null;

  @ManyToOne(() => Whitelist, (whitelist) => whitelist.whitelistLists)
  @JoinColumn([{ name: "whitelist_id", referencedColumnName: "id" }])
  whitelist: Whitelist;
}
