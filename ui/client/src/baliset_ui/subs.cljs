(ns baliset-ui.subs
  (:require [re-frame.core :as rf]))

(rf/reg-sub
 :all-node-metadata
 (fn [db _]
   (:node-metadata db)))

(rf/reg-sub
 :node-types
 (fn [_ _]
   (rf/subscribe [:all-node-metadata]))
 (fn [node-metadata _]
   (map str (keys node-metadata))))

rf/subscribe
(rf/reg-sub
 :node-metadata
 (fn [_ _]
   (rf/subscribe [:all-node-metadata]))
 (fn [node-metadata [_ node-type]]
   (get node-metadata node-type)))

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
 :selected?
 (fn [db [_ inlet-or-outlet node-id inlet-idx]]
   (= (:selected-io db)
      [inlet-or-outlet node-id inlet-idx])))

(rf/reg-sub
 :node-position
 (fn [[_ node-id] _]
   [(rf/subscribe [:node node-id])
    (rf/subscribe [:node-offset node-id])])
 (fn [[node node-offset] _]
   (let [{:keys [x y]} node
         [drag-x drag-y] node-offset]
     [(+ x drag-x) (+ y drag-y)])))

(rf/reg-sub
 :connections
 (fn [db _]
   (:connections db)))

(rf/reg-sub
 :outlet-offset
 (fn [db [_ node-id idx]]
   (get-in db [:outlet-offset node-id idx])))

(rf/reg-sub
 :inlet-offset
 (fn [db [_ node-id idx]]
   (get-in db [:inlet-offset node-id idx])))

(rf/reg-sub
 :connection
 (fn [[_ [out-node-id outlet-idx in-node-id inlet-idx]] _]
   [(rf/subscribe [:node-position out-node-id])
    (rf/subscribe [:node-position in-node-id])
    (rf/subscribe [:outlet-offset out-node-id outlet-idx])
    (rf/subscribe [:inlet-offset in-node-id inlet-idx])])
 (fn [[out-pos in-pos out-offset in-offset] _]
   (let [[out-node-x out-node-y] out-pos
         [in-node-x in-node-y] in-pos
         [out-offset-top out-offset-height out-offset-left out-offset-width] out-offset
         [in-offset-top in-offset-height] in-offset
         out-x (+ out-node-x out-offset-left out-offset-width)
         out-y (+ out-node-y out-offset-top (* out-offset-height 0.5))
         in-x in-node-x
         in-y (+ in-node-y in-offset-top (* in-offset-height 0.5))
         cx1 (+ in-x (/ (- out-x in-x) 2))
         cy1 in-y
         cx2 cx1
         cy2 out-y]
     [out-x out-y cx1 cy1 cx2 cy2 in-x in-y])))

(rf/reg-sub
 :pan
 (fn [db _]
   (:pan db)))

(rf/reg-sub
 :pan-offset
 (fn [db _]
   (:pan-offset db)))

(rf/reg-sub
 :pan-pos
 (fn [_ _]
   [(rf/subscribe [:pan])
    (rf/subscribe [:pan-offset])])
 (fn [[pan pan-offset] _]
   (mapv + pan pan-offset)))
