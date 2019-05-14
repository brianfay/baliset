(ns baliset-ui.subs
  (:require [re-frame.core :as rf]))

(rf/reg-sub
 :node-types
 (fn [db _]
   (map str (keys (:node-metadata db)))))

(rf/reg-sub
 :node-metadata
 (fn [db [_ type]]
   (get (:node-metadata db) type)))

(rf/reg-sub
 :node-ids
 (fn [db _]
   (keys (:nodes db))))

(rf/reg-sub
 :node
 (fn [db [_ id]]
   (get (:nodes db) id)))

(rf/reg-sub
 :node-offset
 (fn [db [_ id]]
   (or (get (:node-offset db) id)
       [0 0])))

(rf/reg-sub
 :pan
 (fn [db _]
   (mapv + (:pan db) (:pan-offset db))))
