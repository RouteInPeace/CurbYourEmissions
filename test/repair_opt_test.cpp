#include "cye/repair_opt.hpp"
#include <gtest/gtest.h>
#include <chrono>
#include <limits>
#include <print>
#include <random>
#include <ranges>
#include <stdexcept>
#include <vector>
#include "cye/init_heuristics.hpp"
#include "cye/repair.hpp"
#include "cye/solution.hpp"

TEST(RepairOpt, Dominates) {
  auto s1 = cye::SolutionCandidate{1, 1, 1};
  auto s2 = cye::SolutionCandidate{3, 3, 3};
  auto s3 = cye::SolutionCandidate{3, 2, 2};

  EXPECT_TRUE(s1.dominates(s2));
  EXPECT_FALSE(s1.is_dominated_by(s2));

  EXPECT_FALSE(s1.dominates(s1));
  EXPECT_FALSE(s1.is_dominated_by(s1));

  EXPECT_FALSE(s2.dominates(s3));
  EXPECT_FALSE(s3.is_dominated_by(s2));
}

TEST(RepairOpt, ParetoFront) {
  std::random_device rd;
  std::mt19937 gen(rd());

  const auto point_cnt = 1000;
  const auto iter = 1000;
  auto dist = std::uniform_real_distribution(0.0, 10.0);

  for (auto i = 0; i < iter; ++i) {
    // Generate random points
    std::vector<cye::SolutionCandidate> points;
    for (auto j = 0; j < point_cnt; ++j) {
      points.emplace_back(dist(gen), dist(gen), dist(gen));
    }

    // Brute force sol
    std::vector<cye::SolutionCandidate> brute_sol;
    for (const auto &p1 : points) {
      bool non_dominated = true;

      for (const auto &p2 : points) {
        if (p1.is_dominated_by(p2)) {
          non_dominated = false;
          break;
        }
      }

      if (non_dominated) {
        brute_sol.push_back(p1);
      }
    }

    // Sort for comparison
    std::ranges::sort(brute_sol, [](const auto &a, const auto &b) {
      if (a.distance != b.distance) return a.distance < b.distance;
      if (a.battery_used != b.battery_used) return a.battery_used < b.battery_used;

      return a.cargo_used < b.cargo_used;
    });

    // Solve
    auto front = cye::ParetoFront();
    for (const auto &p : points) front.emplace(p);
    front.colapse();

    EXPECT_EQ(front.size(), brute_sol.size());

    // Verify
    for (const auto &[p1, p2] : std::ranges::zip_view(brute_sol, front)) {
      EXPECT_EQ(p1.distance, p2.distance);
      EXPECT_EQ(p1.battery_used, p2.battery_used);
      EXPECT_EQ(p1.cargo_used, p2.cargo_used);
    }
  }
}

TEST(RepairOpt, ParetoFrontInvalid) {
  auto front = cye::ParetoFront();
  front.emplace(1.0, 1.0, 1.0);

  EXPECT_THROW((void)*front.begin(), std::runtime_error);
}

TEST(RepairOpt, CsMatrix_Basic) {
  auto archive = serial::JSONArchive("dataset/json/X-n143-k7.json");
  auto instance = std::make_shared<cye::Instance>(archive.root());
  auto mat = cye::CsMatrix(instance);

  auto cs_cnt = instance->charging_station_cnt() + 1;

  for (auto i = 0UZ; i < cs_cnt; ++i) {
    EXPECT_EQ(mat.distance(i, i), 0.0);

    for (auto j = i + 1; j < cs_cnt; ++j) {
      EXPECT_EQ(mat.distance(i, j), mat.distance(j, i));
    }
  }

  // auto cs_cnt = instance->charging_station_cnt() + 1;

  // for (auto i = 0UZ; i < cs_cnt; ++i) {
  //   for (auto j = 0UZ; j < cs_cnt; ++j) {
  //     std::print("{:8.2f}", mat.distance_(i, j));
  //   }
  //   std::println("");
  // }
}

TEST(RepairOpt, CsMatrix_Hard) {
  auto archive = serial::JSONArchive("dataset/json/cs-mat-test.json");
  auto instance = std::make_shared<cye::Instance>(archive.root());
  auto mat = cye::CsMatrix(instance);

  EXPECT_DOUBLE_EQ(mat.distance(0, 4), 120.0);
  EXPECT_DOUBLE_EQ(mat.distance(1, 4), 90.0);
  EXPECT_DOUBLE_EQ(mat.distance(2, 4), 60.0);
}

TEST(RepairOpt, RepairOptTmp) {
  auto archive = serial::JSONArchive("dataset/json/E-n22-k4.json");
  auto instance = std::make_shared<cye::Instance>(archive.root());

  auto repair = cye::OptimalRepair(instance);
  auto solution = cye::nearest_neighbor(instance);

  auto solution_copy = solution;
  cye::linear_split(solution_copy);
  auto energy_repair = cye::OptimalEnergyRepair(instance);
  energy_repair.patch(solution_copy, 1001u);

  for (const auto n : solution_copy.routes()) {
    std::print("{:4}", n);
  }
  std::println("\n{}", solution_copy.cost());

  repair.patch(solution, std::numeric_limits<double>::infinity());
  EXPECT_TRUE(solution.is_cargo_valid());
  EXPECT_TRUE(solution.is_energy_and_cargo_valid());
  EXPECT_TRUE(solution.is_valid());

  for (const auto n : solution.routes()) {
    std::print("{:6}", n);
  }
  std::println();

  auto cargo = 0;
  for (const auto n : solution.routes()) {
    cargo += instance->demand(n);
    if (n == instance->depot_id()) {
      cargo = 0;
    }
    std::print("{:6}", cargo);
  }
  std::println("\n{}", instance->cargo_capacity());
}

TEST(RepairOpt, RepairOpt2) {
  std::random_device rd;
  std::mt19937 gen(1);

  // auto path = "dataset/json/E-n30-k3.json";

  auto max_diff = 0.0;
  auto max_time = 0Z;

  for (const auto &path : std::filesystem::directory_iterator("dataset/json")) {
    auto archive = serial::JSONArchive(path);
    auto instance = std::make_shared<cye::Instance>(archive.root());
    auto energy_repair = cye::OptimalEnergyRepair(instance);
    auto opt_repair = cye::OptimalRepair(instance);

    auto routes = std::vector<size_t>();
    for (auto c : instance->customer_ids()) {
      routes.push_back(c);
    }

    std::println("{}", path.path().c_str());

    for (auto i = 0UZ; i < 10UZ; i++) {
      std::shuffle(routes.begin() + 1, routes.end() - 1, gen);

      auto copy = routes;
      auto copy2 = routes;

      auto solution_bilevel = cye::Solution(instance, std::move(copy));
      cye::linear_split(solution_bilevel);
      energy_repair.patch(solution_bilevel, 101u);

      auto solution_opt = cye::Solution(instance, std::move(copy2));

      auto start_t = std::chrono::high_resolution_clock::now();
      opt_repair.patch(solution_opt, std::numeric_limits<double>::infinity());
      auto end_t = std::chrono::high_resolution_clock::now();

      auto t = std::chrono::duration_cast<std::chrono::milliseconds>(end_t - start_t).count();
      max_time = std::max(max_time, t);

      // std::println("Iteration: {}", i);

      // for (const auto n : solution_bilevel.routes()) {
      //   std::print("{:4}", n);
      // }
      // std::println("\n{}", solution_bilevel.cost());

      // for (const auto n : solution_opt.routes()) {
      //   std::print("{:4}", n);
      // }
      // std::println("\n{}", solution_opt.cost());

      EXPECT_TRUE(solution_bilevel.is_valid());
      EXPECT_TRUE(solution_opt.is_valid());

      EXPECT_LE(solution_opt.cost() - solution_bilevel.cost(), 1e-6);

      max_diff = std::max(max_diff, (solution_bilevel.cost() - solution_opt.cost()) / solution_bilevel.cost());
    }
    std::println("Max diff: {}, Max Time: {}", max_diff, max_time);

    // break;
  }
}

TEST(RepairOpt, RepairOpt) {
  std::random_device rd;
  std::mt19937 gen(1);

  auto path = "dataset/json/X-n916-k207.json";

  auto archive = serial::JSONArchive(path);
  auto instance = std::make_shared<cye::Instance>(archive.root());
  auto opt_repair = cye::OptimalRepair(instance);

  auto routes = std::vector<size_t>{
      492, 701, 506, 712, 345, 182, 551, 536, 502, 696, 737, 703, 193, 470, 778, 465, 705, 71,  32,  624, 607, 517, 363,
      857, 544, 543, 484, 317, 623, 860, 833, 569, 728, 164, 122, 187, 281, 283, 767, 333, 247, 212, 633, 693, 1,   429,
      155, 15,  259, 477, 801, 756, 472, 254, 834, 486, 495, 721, 458, 194, 784, 667, 850, 648, 143, 702, 300, 100, 154,
      35,  174, 273, 17,  208, 453, 845, 45,  774, 327, 628, 637, 804, 647, 218, 441, 49,  222, 406, 704, 819, 815, 874,
      481, 724, 851, 504, 744, 887, 292, 783, 909, 354, 802, 690, 540, 531, 153, 571, 732, 103, 134, 268, 561, 293, 114,
      664, 542, 387, 37,  669, 533, 736, 322, 788, 846, 793, 668, 891, 514, 898, 312, 817, 29,  707, 713, 445, 611, 915,
      500, 368, 592, 386, 853, 172, 192, 83,  310, 33,  516, 796, 338, 563, 809, 362, 577, 509, 836, 211, 616, 764, 753,
      890, 868, 512, 681, 404, 642, 420, 201, 56,  436, 185, 866, 359, 800, 388, 663, 266, 557, 820, 461, 765, 127, 692,
      900, 904, 513, 794, 894, 771, 603, 483, 893, 735, 90,  596, 179, 157, 439, 48,  361, 443, 527, 196, 660, 869, 877,
      825, 760, 619, 365, 532, 706, 489, 742, 782, 520, 367, 638, 843, 695, 878, 645, 88,  790, 749, 711, 590, 10,  776,
      766, 852, 885, 680, 427, 741, 863, 265, 539, 267, 811, 618, 644, 720, 653, 198, 396, 446, 570, 485, 257, 403, 537,
      180, 854, 675, 479, 350, 111, 626, 899, 911, 761, 503, 5,   129, 678, 847, 151, 558, 176, 828, 334, 299, 216, 684,
      438, 347, 892, 52,  243, 85,  335, 578, 545, 602, 555, 91,  573, 595, 879, 785, 913, 16,  171, 630, 612, 872, 698,
      369, 117, 677, 395, 754, 482, 870, 418, 464, 714, 89,  632, 672, 463, 501, 587, 837, 469, 58,  65,  694, 397, 455,
      207, 138, 169, 195, 419, 183, 162, 456, 130, 895, 787, 818, 601, 291, 550, 226, 687, 743, 61,  7,   381, 80,  329,
      250, 799, 373, 282, 699, 676, 831, 430, 146, 816, 454, 304, 101, 120, 197, 634, 405, 87,  435, 210, 190, 394, 3,
      457, 4,   116, 276, 447, 188, 107, 31,  206, 425, 689, 524, 583, 269, 827, 24,  125, 270, 30,  217, 26,  131, 450,
      393, 173, 118, 343, 242, 20,  377, 128, 258, 494, 25,  232, 260, 140, 133, 426, 286, 326, 168, 70,  158, 858, 625,
      409, 656, 132, 148, 789, 156, 74,  42,  82,  302, 364, 620, 119, 823, 505, 747, 873, 748, 639, 449, 63,  849, 165,
      529, 697, 490, 750, 731, 109, 290, 214, 62,  108, 251, 650, 627, 629, 398, 353, 246, 355, 113, 94,  432, 408, 79,
      309, 379, 641, 862, 487, 670, 219, 321, 344, 123, 375, 480, 274, 806, 896, 252, 163, 221, 416, 238, 230, 96,  358,
      75,  160, 658, 812, 314, 584, 876, 279, 97,  341, 390, 19,  126, 428, 76,  324, 236, 791, 325, 886, 220, 278, 57,
      237, 34,  342, 389, 205, 23,  757, 240, 384, 144, 905, 880, 708, 69,  725, 316, 399, 320, 401, 382, 813, 861, 700,
      803, 522, 636, 739, 730, 60,  499, 203, 6,   498, 883, 451, 215, 8,   73,  565, 229, 253, 635, 662, 298, 340, 727,
      572, 473, 523, 609, 856, 475, 383, 442, 294, 380, 28,  875, 805, 337, 93,  777, 556, 328, 462, 576, 385, 631, 752,
      751, 666, 646, 797, 589, 415, 44,  142, 199, 295, 184, 55,  54,  43,  22,  296, 288, 178, 606, 673, 554, 332, 213,
      68,  682, 227, 137, 604, 349, 53,  599, 768, 588, 170, 177, 351, 115, 18,  150, 433, 715, 515, 376, 255, 331, 339,
      424, 11,  189, 223, 372, 614, 859, 248, 200, 149, 585, 46,  209, 686, 112, 12,  525, 608, 717, 474, 746, 530, 471,
      346, 685, 770, 844, 307, 564, 519, 371, 370, 348, 839, 159, 440, 231, 161, 27,  84,  392, 72,  305, 830, 124, 36,
      21,  284, 547, 13,  77,  271, 655, 306, 141, 352, 225, 261, 553, 245, 826, 39,  264, 722, 202, 224, 262, 906, 723,
      263, 289, 580, 779, 493, 431, 810, 510, 417, 181, 228, 67,  99,  303, 285, 738, 912, 136, 422, 95,  357, 272, 234,
      360, 9,   598, 581, 280, 566, 434, 391, 824, 910, 559, 719, 838, 437, 726, 780, 654, 848, 792, 104, 235, 186, 92,
      318, 40,  356, 311, 233, 51,  908, 444, 541, 535, 249, 759, 552, 414, 135, 277, 413, 166, 864, 256, 740, 191, 374,
      98,  674, 574, 605, 301, 175, 534, 407, 400, 795, 786, 640, 14,  78,  562, 661, 807, 560, 881, 643, 649, 840, 308,
      651, 239, 423, 64,  579, 38,  907, 526, 718, 855, 835, 821, 549, 829, 488, 421, 729, 762, 902, 460, 688, 297, 622,
      652, 110, 241, 287, 167, 244, 586, 591, 775, 884, 497, 313, 617, 665, 466, 491, 758, 842, 613, 755, 832, 621, 657,
      546, 734, 600, 528, 145, 508, 914, 378, 366, 66,  330, 319, 763, 323, 745, 841, 710, 59,  716, 769, 808, 709, 402,
      467, 538, 691, 683, 204, 2,   798, 412, 86,  41,  548, 772, 597, 814, 882, 897, 659, 459, 615, 888, 671, 582, 315,
      336, 147, 452, 507, 822, 594, 733, 567, 152, 518, 568, 867, 901, 496, 121, 511, 476, 889, 593, 410, 47,  871, 903,
      773, 781, 468, 478, 575, 610, 275, 105, 521, 411, 139, 106, 50,  448, 102, 81,  679, 865};
  auto solution = cye::Solution(instance, std::move(routes));
  opt_repair.patch(solution, std::numeric_limits<double>::infinity());
  EXPECT_TRUE(solution.is_valid());
  std::println("{}", solution.cost());
}